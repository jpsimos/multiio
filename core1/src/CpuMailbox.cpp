/*
 * CpuMailbox.cpp
 *
 *  Created on: Feb 7, 2025
 *      Author: jacob a psimos
 */


#include "InterruptManager.h"
#include "CpuMailbox.h"
#include "CpuMessage.h"
#include "xil_mmu.h"
#include "xmbox.h"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>


CpuMailbox* CpuMailbox::CreateInstance(
	const unsigned short mboxDeviceId,
	const unsigned short mboxIntrId
)
{
	if(nullptr == sm_cpuMailboxPtr)
	{
		sm_cpuMailboxPtr = new CpuMailbox(mboxDeviceId, mboxIntrId);
	}
	return sm_cpuMailboxPtr;
}

CpuMailbox* CpuMailbox::GetInstance()
{
	if(nullptr == sm_cpuMailboxPtr)
	{
		RUNTIME_ERROR("Must call CreateInstance first");
	}
	return sm_cpuMailboxPtr;
}

CpuMailbox::CpuMailbox(
	const unsigned short mboxDeviceId,
	const unsigned short mboxIntrId
) :
	Middleware(Middleware::AbstractType::CPU_MAILBOX, MW_PRIORITY_HIGH),
	m_mboxIntrId(mboxIntrId),
	m_handlerMap(),
	m_mboxConfig(XMbox_LookupConfig(mboxDeviceId)),
	m_mbox(),
	m_hadError(false),
	m_sendQueue(),
	m_receiveQueue(),
	m_receiveQueueAux(),
	m_receiveQueuePtr(&m_receiveQueue)
{
	int result;

	result = XMbox_CfgInitialize(&m_mbox, m_mboxConfig, m_mboxConfig->BaseAddress);

	if(XST_SUCCESS != result)
	{
		RUNTIME_ERROR("XMbox_CfgInitialize = %d", result);
	}

	XMbox_SetInterruptEnable(&m_mbox, 0);
	XMbox_ClearInterrupt(&m_mbox, XMB_IX_RTA | XMB_IX_STA | XMB_IX_ERR);
	XMbox_SetSendThreshold(&m_mbox, (sizeof(CpuMessage::Message) / 4) - 1);
	XMbox_SetReceiveThreshold(&m_mbox, (sizeof(CpuMessage::Message) / 4) - 1);
	InterruptManager::GetInstance()->Attach(m_mboxIntrId, InterruptManager::IntrType::PL_INTR,
		reinterpret_cast<Xil_InterruptHandler>(CpuMailbox::IntrHandler), this, true);
	XMbox_SetInterruptEnable(&m_mbox, XMB_IX_RTA | XMB_IX_STA | XMB_IX_ERR);
}

CpuMailbox::~CpuMailbox()
{
	XMbox_SetInterruptEnable(&m_mbox, 0);
	InterruptManager::GetInstance()->Detach(m_mboxIntrId, InterruptManager::IntrType::PL_INTR);
}

void CpuMailbox::IntrHandler(CpuMailbox* cpuMailboxPtr)
{
	unsigned int intrStatus;
	CpuMessage* cpuMessagePtr;
	LiteQueue<CpuMessage, CPU_MAILBOX_QUEUE_SIZE>* receiveQueuePtr;

	intrStatus = XMbox_GetInterruptStatus(&(cpuMailboxPtr->m_mbox));

	XMbox_ClearInterrupt(&(cpuMailboxPtr->m_mbox), intrStatus);

	receiveQueuePtr = cpuMailboxPtr->m_receiveQueuePtr == &(cpuMailboxPtr->m_receiveQueue)
		? &(cpuMailboxPtr->m_receiveQueueAux) : &(cpuMailboxPtr->m_receiveQueue);

	if(intrStatus & XMB_IX_RTA)
	{
		if(receiveQueuePtr->prepush(&cpuMessagePtr, true))
		{
			Xil_DCacheInvalidateRange(
				reinterpret_cast<INTPTR>(&(cpuMessagePtr->GetInternalMessagePointer()->u_Raw[0])),
				sizeof(CpuMessage::Message)
			);

			XMbox_ReadBlocking(&(cpuMailboxPtr->m_mbox),
				(u32*)(&(cpuMessagePtr->GetInternalMessagePointer()->u_Raw[0])),
				sizeof(CpuMessage::Message)
			);

			(*cpuMessagePtr)->u_Message.ReceivedTimestamp = Time::Micros();
		}
	}

	if(intrStatus & XMB_IX_ERR)
	{
		cpuMailboxPtr->m_hadError = true;
	}
}

void CpuMailbox::RegisterMessageHandler(const CpuMessage::MessageType type, CpuMessageHandlerFunc func, void* arg)
{
	CpuMessageHandlerMap::iterator handlerFuncWithArgPair = m_handlerMap.find(type);

	if(m_handlerMap.end() == handlerFuncWithArgPair)
	{
		std::vector<CpuMessageHandlerFuncWithArgPair> handlerVec;
		handlerVec.push_back({func, arg});
		m_handlerMap[type] = handlerVec;
	}
	else
	{
		handlerFuncWithArgPair->second.push_back({func, arg});
	}
}

void CpuMailbox::ProcessInbox()
{
	LiteQueue<CpuMessage, CPU_MAILBOX_QUEUE_SIZE>* receiveQueuePtr = m_receiveQueuePtr;
	CpuMessageHandlerMap::iterator handlerFuncVec;

	if(receiveQueuePtr->empty())
	{
		receiveQueuePtr = receiveQueuePtr == &m_receiveQueue ? &m_receiveQueueAux : &m_receiveQueue;
		m_receiveQueuePtr = receiveQueuePtr;
	}

	while(!receiveQueuePtr->empty())
	{
		if(receiveQueuePtr->front().IsValid())
		{
			handlerFuncVec = m_handlerMap.find(receiveQueuePtr->front()->u_Message.Header.Type);

			if(m_handlerMap.end() != handlerFuncVec)
			{
				for(CpuMessageHandlerFuncWithArgPair funcArgPair : handlerFuncVec->second)
				{
					if(!funcArgPair.first(receiveQueuePtr->front(), funcArgPair.second))
					{
						DEBUG_LOG("Handler for type %s returned false.",
							receiveQueuePtr->front().GetMessageTypeName());
					}
				}
			}
		}
		else
		{
			DEBUG_ERROR("Received invalid message with ID: %s",
				receiveQueuePtr->front().GetMessageTypeName());
		}

		receiveQueuePtr->pop();
	}
}

void CpuMailbox::ProcessOutbox()
{
	if(!m_sendQueue.empty() && !XMbox_IsFull(&m_mbox))
	{
		m_sendQueue.front().UpdateChecksum();
		XMbox_WriteBlocking(
			&m_mbox,
			(u32*)(&(m_sendQueue.front()->u_Raw[0])),
			sizeof(CpuMessage::Message)
		);

		m_sendQueue.pop();
	}
}

bool CpuMailbox::QueueMessage(CpuMessage& cpuMsg)
{
	return m_sendQueue.push(cpuMsg);
}

void CpuMailbox::Loop()
{
	if(m_hadError)
	{
		DEBUG_ERROR("Status = 0x%x", XMbox_GetStatus(&m_mbox));
		m_hadError = false;
	}
	ProcessInbox();
	ProcessOutbox();
}

int CpuMailbox::ProcessCommand(const std::string& command)
{
	int commandResult = COMMAND_IGNORE;

	if(command == "mbox test")
	{
		CpuMessage testMsg(CpuMessage::MessageType::Test);
		QueueMessage(testMsg);
	}

	return commandResult;
}

