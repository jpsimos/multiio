/*
 * CpuMessage.cpp
 *
 *  Created on: Aug 12, 2023
 *      Author: JacobAPsimos
 */

#include <string>
#include <cstring>
#include <stdexcept>
#include "xparameters.h"
#include "xil_cache.h"
#include "CpuMessage.h"
#include "Helpers.h"

using namespace Helpers;

CpuMessage::CpuMessage() :
	m_Message()
{
}

CpuMessage::CpuMessage(const CpuMessage::MessageType msgType) :
	m_Message()
{
	m_Message.u_Message.Header.Type = msgType;
}

CpuMessage::CpuMessage(const CpuMessage::Message* msgPtr) :
	m_Message(msgPtr)
{
}

CpuMessage::Message* CpuMessage::operator->()
{
	return &m_Message;
}

CpuMessage::Message* CpuMessage::GetInternalMessagePointer()
{
	return &m_Message;
}

const CpuMessage::Message* CpuMessage::GetConstInternalMessagePointer() const
{
	return &m_Message;
}

void CpuMessage::UpdateMessageId(unsigned int& messageId)
{
	messageId += 2;
	m_Message.u_Message.Header.MessageId = messageId;
}

void CpuMessage::UpdateChecksum()
{
	m_Message.u_Message.Checksum = CpuMessage::CalculateChecksum(&m_Message);
}

void CpuMessage::UpdateTimestamp()
{
	m_Message.u_Message.Header.Timestamp = Time::Micros();
}

void CpuMessage::UpdateReceivedTimestamp()
{
	m_Message.u_Message.ReceivedTimestamp = Time::Micros();
}

bool CpuMessage::IsValid() const
{
	return static_cast<bool>(
		Time::Micros() >= m_Message.u_Message.Header.Timestamp &&
		Time::Micros() >= m_Message.u_Message.ReceivedTimestamp &&
		m_Message.u_Message.Checksum == CpuMessage::CalculateChecksum(&m_Message)
	);
}

std::string CpuMessage::ToString() const
{
	std::ostringstream ss;

	ss << ANSI_YELLOW
		<< "ID:" << m_Message.u_Message.Header.MessageId
		<< ", Type:" << static_cast<unsigned int>(m_Message.u_Message.Header.Type) << " " << GetMessageTypeName()
		<< ", Timestamp:" << m_Message.u_Message.Header.Timestamp
		<< ", ReceivedTimestamp:" << m_Message.u_Message.ReceivedTimestamp
		<< ", Checksum:" << std::hex << m_Message.u_Message.Checksum
		<< ", Valid:" << IsValid()
		<< ANSI_RESET;


	return ss.str();
}

const char* const CpuMessage::GetMessageTypeName() const
{
	if(static_cast<unsigned int>(m_Message.u_Message.Header.Type)
		< sizeof(CpuMessage::MessageTypeNames) / sizeof(const char*))
	{
		return CpuMessage::MessageTypeNames[static_cast<unsigned int>(m_Message.u_Message.Header.Type)];
	}

	return NULL;
}

uint32_t CpuMessage::CalculateChecksum(const CpuMessage::Message* msgPtr)
{
	return Checksum::CRC32::Recalculate(Checksum::CRC32::Calculate(&(msgPtr->u_Message.Header),
		sizeof(CpuMessage::MessageHeader)), &(msgPtr->u_Message.Body), sizeof(CpuMessage::MessageBody));
}
