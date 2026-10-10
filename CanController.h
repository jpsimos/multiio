/*
 * CanController.h
 *
 *  Created on: May 18, 2025
 *      Author: jacob a psimos
 */

#ifndef SRC_CANCONTROLLER_H_
#define SRC_CANCONTROLLER_H_

#include <array>
#include <algorithm>
#include <string>
#include "LiteQueue.hpp"
#include "Helpers.h"
#include "Middleware.h"
#include "xparameters.h"
#include "xcanps.h"

#define NUM_CAN_CONTROLLERS XPAR_XCANPS_NUM_INSTANCES
#define CAN_CONTROLLER_QUEUE_SIZE 64

using namespace Helpers;

class CanController: public Middleware
{
public:
	class Message
	{
	public:
		union Frame
		{
			struct
			{
				uint32_t m_id;
				uint32_t m_dlc;
				uint8_t m_data[8];
			} s_frame;
			uint32_t u_dwords[XCANPS_MAX_FRAME_SIZE / 4];
			uint8_t u_bytes[XCANPS_MAX_FRAME_SIZE];
		};

		Message();
		Message(
			uint32_t id,
			uint8_t length,
			uint8_t* data
		);
		Message(const Message& other);
		~Message() {}
		bool operator==(const Message& other) const;
		Frame* operator->();
		Frame* GetInternalFramePointer();
		const Frame* GetConstInternalFramePointer() const;
		std::string ToString(const unsigned short phy = std::numeric_limits<unsigned short>::max()) const;
		uint16_t GetStandardId() const;
		uint16_t GetExtendedId() const;
		uint8_t GetLength() const;

	private:
		alignas(32) CanController::Message::Frame m_frame;
	};

	static CanController* CreateInstance(
		const unsigned short deviceId,
		const unsigned char intrId
	);

	static CanController* GetInstance(const unsigned short deviceId);

private:
	CanController(
		const unsigned short deviceId,
		const unsigned char intrId
	);
	virtual ~CanController();

public:
	bool SetBaudRate(unsigned int baudRate);
	bool Send(CanController::Message& msg);
	bool Receive(CanController::Message& msg);
	void ProcessInbox();
	virtual void Loop();
	virtual int ProcessCommand(const std::string& command);

private:
	static void CanSentIsr(CanController* canPtr);
	static void CanReceivedIsr(CanController* canPtr);
	static void CanErrorIsr(CanController* canPtr, unsigned int errorMask);
	static void CanEventIsr(CanController* canPtr, unsigned int eventMask);

private:
	const unsigned short m_deviceId;
	const unsigned char m_intrId;
	XCanPs_Config* m_canConfig;
	XCanPs m_can;
	volatile unsigned int m_numReceivedIntrs;
	volatile unsigned int m_numSentIntrs;
	volatile unsigned int m_numErrorIntrs;
	LiteQueue<CanController::Message, CAN_CONTROLLER_QUEUE_SIZE> m_receiveQueue;
	LiteQueue<CanController::Message, CAN_CONTROLLER_QUEUE_SIZE> m_receiveQueueAux;
	LiteQueue<CanController::Message, CAN_CONTROLLER_QUEUE_SIZE>* m_receiveQueuePtr;
	bool m_processingInbox;
};

inline CanController::Message::Message()
{
	std::memset(&m_frame.u_bytes[0], '\0', sizeof(CanController::Message::Frame));
}

inline CanController::Message::Message(
	uint32_t id,
	uint8_t length,
	uint8_t* data
)
{
	m_frame.s_frame.m_id = id;
	m_frame.s_frame.m_dlc = XCanPs_CreateDlcValue(static_cast<uint32_t>(length));

	if(nullptr != &data[0])
	{
		std::memcpy(&m_frame.s_frame.m_data[0], &data[0],
			std::min(static_cast<size_t>(length), sizeof(m_frame.s_frame.m_data)));
	}
}

inline CanController::Message::Message(const CanController::Message& other)
{
	std::memcpy(&m_frame.u_bytes[0], &other.m_frame.u_bytes[0],
		sizeof(CanController::Message::Frame));
}

inline bool CanController::Message::operator==(const CanController::Message& other) const
{
	return static_cast<bool>(!std::memcmp(&m_frame.u_bytes[0],
		&other.m_frame.u_bytes[0], sizeof(CanController::Message::Frame)));
}

inline CanController::Message::Frame* CanController::Message::operator->()
{
	return &m_frame;
}

inline const CanController::Message::Frame* CanController::Message::GetConstInternalFramePointer() const
{
	return &m_frame;
}

inline std::string CanController::Message::ToString(const unsigned short phy) const
{
	std::ostringstream oss;

	oss << "{"
		<< std::quoted("phy") << ":" << std::dec << static_cast<unsigned int>(phy) << ","
		<< std::quoted("idr") << ":0x" << std::hex << static_cast<unsigned int>(m_frame.s_frame.m_id) << ","
		<< std::quoted("ids") << ":0x" << std::hex << static_cast<unsigned int>(GetStandardId()) << ","
		<< std::quoted("ide") << ":0x" << std::hex << static_cast<unsigned int>(GetExtendedId()) << ","
		<< std::quoted("dlc") << ":0x" << std::hex << static_cast<unsigned int>(m_frame.s_frame.m_dlc) << ","
		<< std::quoted("dlen") << ":" << std::dec << static_cast<unsigned int>(GetLength()) << ","
		<< std::quoted("data") << ":[";

	for(unsigned int i = 0; i < 8; i++)
	{
		oss << "0x" << std::hex << static_cast<unsigned int>(m_frame.s_frame.m_data[i]);
		oss << ((i < 7) ? "," : "]");
	}

	oss << "}";

	return oss.str();
}

inline uint16_t CanController::Message::GetStandardId() const
{
	return static_cast<uint16_t>(
		(m_frame.s_frame.m_id & XCANPS_IDR_ID1_MASK) >> XCANPS_IDR_ID1_SHIFT);
}

inline uint16_t CanController::Message::GetExtendedId() const
{
	return static_cast<uint16_t>(
		(m_frame.s_frame.m_id & XCANPS_IDR_ID2_MASK) >> XCANPS_IDR_ID2_SHIFT);
}

inline uint8_t CanController::Message::GetLength() const
{
	return static_cast<uint8_t>(
		(m_frame.s_frame.m_dlc & XCANPS_DLCR_DLC_MASK) >> XCANPS_DLCR_DLC_SHIFT);
}

namespace std
{
	template<>
	struct hash<CanController::Message>
	{
		inline size_t operator()(const CanController::Message& item) const
		{
			return static_cast<size_t>(Checksum::CRC32::Calculate(&(item.GetConstInternalFramePointer()->u_bytes[0]),
				sizeof(CanController::Message::Frame)));
		}
	};
}

#endif /* SRC_CANCONTROLLER_H_ */
