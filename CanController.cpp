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

	for(CanController* iterPtr : sm_cans)
	{
		if(nullptr != iterPtr)
		{
			if(deviceId == iterPtr->m_deviceId)
			{
				canPtr = iterPtr;
				break;
			}
		}
	}

	if(nullptr == canPtr)
	{
		canPtr = new CanController(deviceId, intrId);
	}

	return canPtr;
}

CanController* CanController::GetInstance(const unsigned short deviceId)
{
	CanController* canPtr = nullptr;

	for(CanController* iterPtr : sm_cans)
	{
		if(nullptr != iterPtr)
		{
			if(deviceId == iterPtr->m_deviceId)
			{
				canPtr = iterPtr;
				break;
			}
		}
	}

	if(nullptr == canPtr)
	{
		RUNTIME_ERROR("Must can CreateInstance first");
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
	m_numReceivedIntrs(0U),
	m_numSentIntrs(0U),
	m_numErrorIntrs(0U),
	m_receiveQueue(true),
	m_receiveQueueAux(true),
	m_receiveQueuePtr(&m_receiveQueue),
	m_processingInbox(false)
{
	union
	{
		s32 result;
		u8 mode;
	};

	static_assert(sizeof(CanController::Message::Frame) == sizeof(CanController::Message::Frame::u_bytes),
		"CanController::Message sizes do not match.");

	result = XCanPs_CfgInitialize(&m_can, m_canConfig, m_canConfig->BaseAddr);

	if(XST_SUCCESS != result)
	{
		RUNTIME_ERROR("XCanPs_CfgInitialize = %d", result);
	}

	mode = XCanPs_GetMode(&m_can);

	if(XCANPS_MODE_CONFIG != mode)
	{
		RUNTIME_ERROR("XCanPs_GetMode = %u", mode);
	}

	if(!SetBaudRate(500000))
	{
		DEBUG_ERROR("SetBaudRate failed.");
	}

	XCanPs_SetRxIntrWatermark(&m_can, 32);

	result = XCanPs_SetHandler(&m_can, XCANPS_HANDLER_SEND, (void*)
		reinterpret_cast<XCanPs_SendRecvHandler>(CanController::CanSentIsr), this);
	result |= XCanPs_SetHandler(&m_can, XCANPS_HANDLER_RECV, (void*)
		reinterpret_cast<XCanPs_SendRecvHandler>(CanController::CanReceivedIsr), this);
	result |= XCanPs_SetHandler(&m_can, XCANPS_HANDLER_ERROR, (void*)
		reinterpret_cast<XCanPs_ErrorHandler>(CanController::CanErrorIsr), this);
	result |= XCanPs_SetHandler(&m_can, XCANPS_HANDLER_EVENT, (void*)
		reinterpret_cast<XCanPs_EventHandler>(CanController::CanEventIsr), this);

	if(XST_SUCCESS != result)
	{
		RUNTIME_ERROR("XCanPs_SetHandler... = %d", result);
	}

	InterruptManager::GetInstance()->Attach(
		m_intrId,
		reinterpret_cast<Xil_InterruptHandler>(XCanPs_IntrHandler),
		&m_can,
		false,
		0xA0,
		false
	);
	XCanPs_IntrEnable(&m_can, XCANPS_IXR_RXNEMP_MASK);
	InterruptManager::GetInstance()->Enable(m_intrId);

	XCanPs_EnterMode(&m_can, XCANPS_MODE_NORMAL);
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
	unsigned int clock;
	unsigned int calculatedBaud;
	unsigned int ts1;
	unsigned int ts2;
	unsigned int brp;
	unsigned int sjw;
	unsigned int useTs1;
	unsigned int useTs2;
	unsigned int useBrp;
	unsigned int useSjw;

	static_assert(XPAR_PS7_CAN_0_CAN_CLK_FREQ_HZ == XPAR_PS7_CAN_1_CAN_CLK_FREQ_HZ, "PS CAN Clocks do not match.");

	clock = XPAR_PS7_CAN_0_CAN_CLK_FREQ_HZ;

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
			XCanPs_SetBaudRatePrescaler(&m_can, static_cast<u8>(useBrp));
			XCanPs_SetBitTiming(
				&m_can,
				static_cast<u8>(useSjw),
				static_cast<u8>(useTs2),
				static_cast<u8>(useTs1)
			);
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

bool CanController::Receive(CanController::Message& msg)
{
	bool hadError = true;
	s32 status;

	if(!XCanPs_IsRxEmpty(&m_can))
	{
		status = XCanPs_Recv(&m_can, (u32*)&msg->u_dwords[0]);

		if(XST_SUCCESS == status)
		{
			hadError = false;
		}
	}

	return !hadError;
}

void CanController::CanSentIsr(CanController* canPtr)
{
	if(nullptr != canPtr)
	{
		canPtr->m_numSentIntrs += 1;
	}
}

void CanController::CanReceivedIsr(CanController* canPtr)
{
	CanController::Message* msgPtr;

	if(canPtr != nullptr)
	{
		canPtr->m_numReceivedIntrs += 1;

		if(canPtr->m_receiveQueuePtr->prepush(&msgPtr))
		{
			if(!canPtr->Receive(*msgPtr))
			{
				canPtr->m_receiveQueuePtr->unpush();
			}
		}
	}
}

void CanController::CanErrorIsr(CanController* canPtr, unsigned int errorMask)
{
	if(nullptr != canPtr)
	{
		canPtr->m_numErrorIntrs += 1;
	}
}

void CanController::CanEventIsr(CanController* canPtr, unsigned int eventMask)
{
	return;
}

void CanController::ProcessInbox()
{
	LiteQueue<CanController::Message, CAN_CONTROLLER_QUEUE_SIZE>* receiveQueuePtr;
	CanController::Message* msgPtr;

	receiveQueuePtr = m_receiveQueuePtr;

	if(!XCanPs_IsRxEmpty(&m_can) || !receiveQueuePtr->empty())
	{
		m_receiveQueuePtr = receiveQueuePtr == &m_receiveQueue ? &m_receiveQueueAux : &m_receiveQueue;

		if(!XCanPs_IsRxEmpty(&m_can))
		{
			XCanPs_IntrDisable(&m_can, XCANPS_IXR_RXNEMP_MASK);
			msgPtr = nullptr;

			do
			{
				if(receiveQueuePtr->prepush(&msgPtr))
				{
					if(!Receive(*msgPtr))
					{
						receiveQueuePtr->unpush();
						msgPtr = nullptr;
						break;
					}
				}
				else
				{
					break;
				}
			}
			while(!XCanPs_IsRxEmpty(&m_can));

			XCanPs_IntrEnable(&m_can, XCANPS_IXR_RXNEMP_MASK);
		}

		while(!receiveQueuePtr->empty())
		{
			// std::cout << receiveQueuePtr->front().ToString(m_deviceId) << '\n';
			receiveQueuePtr->pop();
		}
	}
}

void CanController::Loop()
{
	if(!m_processingInbox)
	{
		m_processingInbox = true;
		ProcessInbox();
		m_processingInbox = false;
	}


	static Time::Stopwatch testTimer;
	static unsigned short phy = XPAR_XCANPS_0_DEVICE_ID;

	if(m_deviceId == phy  && testTimer >= 0.25)
	{
		ProcessCommand(Text::Stringf("can %u test", m_deviceId));
		testTimer = 0;
		phy ^= 1;
	}
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
			const unsigned int numSent = m_numSentIntrs;
			const unsigned int numReceived = m_numReceivedIntrs;
			const unsigned int numErrors = m_numErrorIntrs;

			std::cout << ANSI_LIGHT_GREEN "CAN[" << std::dec << m_deviceId << "]" ANSI_RESET "\n" <<
				"\t Num Sent = " << numSent << "\n" <<
				"\t Num Recvd = " << numReceived << "\n" <<
				"\t Num Errs = " << numErrors << "\n" <<
				"\t Mode = " << static_cast<unsigned int>(XCanPs_GetMode(&m_can)) << "\n";

			commandResult = COMMAND_OK;
		}
		else if(Text::CStrEq(subCommandCStr, "test"))
		{
			unsigned int randomLength = Random::Random<unsigned int>(0, 8);

			CanController::Message msg(
				XCanPs_CreateIdValue(Random::Random<unsigned int>(0x001, 0x7F0), 0, 0, 0, 0),
				static_cast<unsigned char>(randomLength),
				nullptr
			);

			for(unsigned int i = 0; i < randomLength; i++)
			{
				static unsigned char dataValue = 0;
				msg->s_frame.m_data[i] = dataValue++;
			}

			Send(msg);

			commandResult = COMMAND_OK;
		}
	}

	return commandResult;
}


