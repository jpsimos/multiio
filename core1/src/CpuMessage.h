/*
 * CpuMessage.h
 *
 *  Created on: Aug 12, 2023
 *      Author: JacobAPsimos
 */

#ifndef SRC_CPUMESSAGE_H_
#define SRC_CPUMESSAGE_H_

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include "Helpers.h"

using namespace Helpers;

//
// Definitions
//
#define CPU_MESSAGE_SIZE 1024

#define CPU_MESSAGE_TYPES \
	CPU_MESSAGE_DEF(Cpu1Loaded)

class CpuMessage
{
	public:
		enum class MessageType : unsigned int
		{
			Test = 0,
			#define CPU_MESSAGE_DEF(type) type,
				CPU_MESSAGE_TYPES
			#undef CPU_MESSAGE_DEF
		};

		static constexpr const char* MessageTypeNames[] = {
			"Test",
			#define CPU_MESSAGE_DEF(type) #type,
				CPU_MESSAGE_TYPES
			#undef CPU_MESSAGE_DEF
		};

		/*  Begin CpuMessage Type Prototypes  */

		struct Cpu1LoadedMessage
		{
			// Indicates the CPU 1 has loaded.
		};
		
		// ...

		/*  End Of CpuMessage Type Prototypes  */

		struct MessageHeader
		{
			CpuMessage::MessageType Type;
			unsigned int SourceCpuId;
			unsigned int MessageId;
			unsigned int ResponseToMessageId;
			long long Timestamp;
		};

		struct MessageBody
		{
			union
			{
				Cpu1LoadedMessage u_cpu1LoadedMessage;
				// ...
			};
		};

		struct alignas(16) Message
		{
			union
			{
				struct
				{
					MessageHeader Header;
					MessageBody Body;
					unsigned int Checksum;
					long long ReceivedTimestamp;
				} u_Message;

				unsigned char u_Raw[CPU_MESSAGE_SIZE];
			};

			Message();
			Message(const Message& otherMsg);
			Message(const Message* otherMsgPtr);
			bool operator==(const Message& otherMsg);
		};


	public:
		CpuMessage();
		CpuMessage(const CpuMessage::MessageType msgType);
		CpuMessage(const CpuMessage::Message* msgPtr);
		~CpuMessage() {}
		CpuMessage::Message* operator->();
		CpuMessage::Message* GetInternalMessagePointer();
		const CpuMessage::Message* GetConstInternalMessagePointer() const;
		void UpdateMessageId(unsigned int& messageId);
		void UpdateChecksum();
		void UpdateTimestamp();
		void UpdateReceivedTimestamp();
		bool IsValid() const;
		std::string ToString() const;
		const char* const GetMessageTypeName() const;
		static uint32_t CalculateChecksum(const CpuMessage::Message* msgPtr);

	private:
		alignas(16) CpuMessage::Message m_Message;
};

inline CpuMessage::Message::Message()
{
	std::memset(&u_Raw[0], '\0', sizeof(CpuMessage::Message));
}

inline CpuMessage::Message::Message(const CpuMessage::Message& otherMsg)
{
	if(&u_Raw[0] != &(otherMsg.u_Raw[0]))
	{
		std::memcpy(&u_Raw[0], &(otherMsg.u_Raw[0]), sizeof(CpuMessage::Message));
	}
}

inline CpuMessage::Message::Message(const CpuMessage::Message* otherMsgPtr)
{
	if(&u_Raw[0] != &(otherMsgPtr->u_Raw[0]))
	{
		std::memcpy(&u_Raw[0], &(otherMsgPtr->u_Raw[0]), sizeof(CpuMessage::Message));
	}
}

inline bool CpuMessage::Message::operator==(const CpuMessage::Message& otherMsg)
{
	const unsigned char* rawPtr = reinterpret_cast<const unsigned char*>(&(u_Message.Header));
	const unsigned char* otherRawPtr = reinterpret_cast<const unsigned char*>(&(otherMsg.u_Message.Header));

	if(!std::memcmp(&rawPtr[0], &otherRawPtr[0], sizeof(CpuMessage::MessageHeader)))
	{
		rawPtr = reinterpret_cast<const unsigned char*>(&(u_Message.Body));
		otherRawPtr = reinterpret_cast<const unsigned char*>(&(otherMsg.u_Message.Body));

		if(!std::memcmp(&rawPtr[0], &otherRawPtr[0], sizeof(CpuMessage::MessageBody)))
		{
			return static_cast<bool>(u_Message.Checksum == otherMsg.u_Message.Checksum);
		}
	}

	return false;
}

namespace std
{
	template<>
	struct hash<CpuMessage>
	{
		inline size_t operator()(const CpuMessage& cpuMsg) const
		{
			return static_cast<size_t>(CpuMessage::CalculateChecksum(cpuMsg.GetConstInternalMessagePointer()));
		}
	};
}

//
//

#endif /* SRC_CPUMESSAGE_H_ */
