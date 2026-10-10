/*
 * CanController.cpp
 *
 *  Created on: May 18, 2025
 *      Author: jacob a psimos
 */

#include <array>
#include "Helpers.h"
#include "InterruptManager.h"
#include "CanController.h"

using namespace Helpers;

static std::array<CanController*, NUM_CAN_CONTROLLERS> sm_cans;

CanController* CanController::CreateInstance(
	const unsigned short deviceId,
	const unsigned char intrId
)
{
	CanController* canPtr = nullptr;

	if(deviceId < NUM_CAN_CONTROLLERS)
	{
		canPtr = sm_cans.at(deviceId);

		if(nullptr == canPtr)
		{
			canPtr = new CanController(deviceId, intrId);
		}
	}

	return canPtr;
}

CanController* CanController::GetInstance(const unsigned short deviceId)
{
	CanController* canPtr = nullptr;

	if(deviceId < NUM_CAN_CONTROLLERS)
	{
		if(nullptr == sm_cans.at(deviceId))
		{
			RUNTIME_ERROR("Must can CreateInstance first");
		}
		canPtr = sm_cans.at(deviceId);
	}

	return canPtr;
}

CanController::CanController(
	const unsigned short deviceId,
	const unsigned char intrId
) :
	Middleware(Middleware::AbstractType::CAN_CONTROLLER, MW_PRIORITY_HIGHER),
	m_deviceId(deviceId),
	m_intrId(intrId),
	m_canConfig(XCanPs_LookupConfig(deviceId)),
	m_can(),
	m_numFramesReceived(0U),
	m_numFramesSent(0U),
	m_numErrors(0U),
	m_receiveQueue(),
	m_receiveQueueAux(),
	m_receiveQueuePtr(&m_receiveQueue)
{
	int result = XST_FAILURE;

	if(sizeof(CanController::Message::Frame) != sizeof(CanController::Message::Frame::u_bytes))
	{
		RUNTIME_ERROR("Message structure is bad. Fix code.");
	}

	result = XCanPs_CfgInitialize(&m_can, m_canConfig, m_canConfig->BaseAddr);
	result |= !!(XCANPS_MODE_CONFIG != XCanPs_GetMode(&m_can)) * XST_NOT_ENABLED;

	if(XST_SUCCESS != result)
	{
		RUNTIME_ERROR("XCanPs_CfgInitialize = %d\n", result);
	}

	SetBaudRate(33333);

	result = XCanPs_SetHandler(&m_can, XCANPS_HANDLER_SEND, (void*)
		reinterpret_cast<XCanPs_SendRecvHandler>(CanController::CanSendIsr), this);
	result |= XCanPs_SetHandler(&m_can, XCANPS_HANDLER_RECV, (void*)
		reinterpret_cast<XCanPs_SendRecvHandler>(CanController::CanReceiveIsr), this);
	result |= XCanPs_SetHandler(&m_can, XCANPS_HANDLER_ERROR, (void*)
		reinterpret_cast<XCanPs_ErrorHandler>(CanController::CanErrorIsr), this);
	result |= XCanPs_SetHandler(&m_can, XCANPS_HANDLER_EVENT, (void*)
		reinterpret_cast<XCanPs_EventHandler>(CanController::CanEventIsr), this);

	if(XST_SUCCESS != result)
	{
		RUNTIME_ERROR("XCanPs_SetHandler... = %d", (int)result);
	}

	InterruptManager::GetInstance()->Attach(
		m_intrId,
		reinterpret_cast<Xil_InterruptHandler>(XCanPs_IntrHandler),
		&m_can,
		false,
		0xA0,
		false
	);
	XCanPs_IntrEnable(&m_can, XCANPS_IXR_ALL);
	XCanPs_SetRxIntrWatermark(&m_can, 1);
	XCanPs_SetTxIntrWatermark(&m_can, 1);
	InterruptManager::GetInstance()->Enable(m_intrId);

	XCanPs_EnterMode(&m_can, XCANPS_MODE_LOOPBACK);
}

CanController::~CanController()
{
	XCanPs_IntrDisable(&m_can, XCANPS_IXR_ALL);
	XCanPs_IntrClear(&m_can, XCANPS_IXR_ALL);
	InterruptManager::GetInstance()->Detach(m_intrId);
}

bool CanController::SetBaudRate(unsigned int baudRate)
{
	bool hadError = true;
	bool found = false;
	unsigned int clock = XPAR_PS7_CAN_0_CAN_CLK_FREQ_HZ;
	unsigned int calculatedBaud;
	unsigned int ts1;
	unsigned int ts2;
	unsigned int brp;
	unsigned int sjw;
	unsigned int useTs1;
	unsigned int useTs2;
	unsigned int useBrp;
	unsigned int useSjw;

	if(XCANPS_MODE_CONFIG == XCanPs_GetMode(&m_can))
	{
		for(brp = 0; brp <= 255 && !found; brp++)
		{
			for(ts1 = 0; ts1 <= 15 && !found; ts1++)
			{
				for(ts2 = 0; ts2 <= 7 && !found; ts2++)
				{
					for(sjw = 3; sjw >= 1 && !found; sjw--)
					{
						calculatedBaud = clock / ((brp + 1) * (sjw + ts1 + ts2));

						if(calculatedBaud == baudRate)
						{
							found = true;
							useTs1 = ts1;
							useTs2 = ts2;
							useBrp = brp;
							useSjw = sjw;
						}
					}
				}
			}
		}

		if(found)
		{
			XCanPs_SetBaudRatePrescaler(&m_can, useBrp);
			XCanPs_SetBitTiming(&m_can, useSjw, useTs2, useTs1);
			hadError = false;
		}
	}

	return !hadError;
}

bool CanController::Send(CanController::Message& msg)
{
	bool hadError = false;
	s32 status;

	if(!XCanPs_IsTxFifoFull(&m_can))
	{
		msg->s_frame.m_dlc &= 0xF0000000;
		status = XCanPs_Send(&m_can, (u32*)&msg->u_dwords[0]);

		if(XST_SUCCESS != status)
		{
			hadError = true;
			DEBUG_ERROR("XCanPs_Send = 0x%x", status);
		}
	}

	return !hadError;
}

void CanController::CanSendIsr(CanController* canPtr)
{
	if(NULL != canPtr)
	{
		canPtr->m_numFramesSent += 1;
	}
}

void CanController::CanReceiveIsr(CanController* canPtr)
{
	s32 status;
	CanController::Message* canMsgPtr;

	if(canPtr->m_receiveQueue.prepush(&canMsgPtr, false))
	{
		status = XCanPs_Recv(&(canPtr->m_can), (u32*)&(*canMsgPtr)->u_dwords[0]);

		if(XST_SUCCESS != status)
		{
			canPtr->m_receiveQueue.unpush();
			canMsgPtr = NULL;
		}

		canPtr->m_numFramesReceived += 1;
	}
}

void CanController::CanErrorIsr(CanController* canPtr, unsigned int errorMask)
{
	if(NULL != canPtr)
	{
		canPtr->m_numErrors += 1;
	}
}

void CanController::CanEventIsr(CanController* canPtr, unsigned int eventMask)
{
	if(NULL != canPtr)
	{
		if(eventMask & XCANPS_IXR_RXOK_MASK)
		{
			nop();
		}
	}
}

void CanController::ProcessInbox()
{
	LiteQueue<CanController::Message, CAN_CONTROLLER_QUEUE_SIZE>* receiveQueuePtr;

	if(!m_receiveQueuePtr->empty())
	{
		receiveQueuePtr = m_receiveQueuePtr;

		XCanPs_IntrDisable(&m_can, XCANPS_IXR_ALL);
		m_receiveQueuePtr = m_receiveQueuePtr == &m_receiveQueue ? &m_receiveQueueAux : &m_receiveQueue;
		XCanPs_IntrEnable(&m_can, XCANPS_IXR_ALL);

		do
		{
			//
		}
		while(!receiveQueuePtr->empty());
	}
}

void CanController::Loop()
{
	ProcessInbox();
}

int CanController::ProcessCommand(const std::string& command)
{
	int commandResult = COMMAND_IGNORE;
	const char* subCommandCStr;
	unsigned int controllerNum;
	int numChars;

	if(std::sscanf(command.c_str(), "can %u %n", &controllerNum, &numChars)
		&& controllerNum == m_deviceId && numChars < static_cast<int>(command.length()))
	{
		subCommandCStr = command.c_str() + numChars;

		if(Text::CStrEq(subCommandCStr, "status"))
		{
			const unsigned int numSent = m_numFramesSent;
			const unsigned int numReceived = m_numFramesReceived;
			const unsigned int numErrors = m_numErrors;

			std::cout << ANSI_LIGHT_GREEN "CAN[" << std::dec << m_deviceId << "]" ANSI_RESET "\n" <<
				"\t Num Sent = " << numSent << "\n" <<
				"\t Num Recvd = " << numReceived << "\n" <<
				"\t Num Errs = " << numErrors << "\n" <<
				"\t Mode = " << static_cast<unsigned int>(XCanPs_GetMode(&m_can)) << "\n";

			commandResult = COMMAND_OK;
		}
		else if(Text::CStrEq(subCommandCStr, "test"))
		{
			s32 status;

			for(unsigned int c = 0; c < 10; c++)
			{
				CanController::Message msg(
					XCanPs_CreateIdValue(Random::Random<unsigned int>(0x001, 0x7F0), 0, 0, 0, 0),
					8,
					NULL
				);

				for(unsigned int i = 0; i < 8; i++)
				{
					static unsigned char dataValue = 0;
					msg->s_frame.m_data[i] = dataValue++;
				}

				if(!XCanPs_IsTxFifoFull(&m_can))
				{
					status = XCanPs_Send(&m_can, (u32*)&msg->u_dwords[0]);

					if(XST_SUCCESS != status)
					{
						DEBUG_ERROR("XCanPs_Send = 0x%x", status);
						break;
					}
				}
			}

			commandResult = COMMAND_OK;
		}
	}

	return commandResult;
}


