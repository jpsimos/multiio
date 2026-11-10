/*
 * CpuMailbox.h
 *
 *  Created on: Feb 7, 2025
 *      Author: jacob a psimos
 */

#ifndef SRC_CPUMAILBOX_H_
#define SRC_CPUMAILBOX_H_

#include "Middleware.h"
#include "Helpers.h"
#include "CpuMessage.h"
#include "xmbox.h"
#include <unordered_map>
#include <functional>
#include "LiteQueue.hpp"

using namespace Helpers;

#define CPU_MAILBOX_QUEUE_SIZE 16

typedef std::function<bool(CpuMessage&, void*)> CpuMessageHandlerFunc;
typedef std::pair<CpuMessageHandlerFunc, void*> CpuMessageHandlerFuncWithArgPair;
typedef std::unordered_map<CpuMessage::MessageType,
	std::vector<CpuMessageHandlerFuncWithArgPair>> CpuMessageHandlerMap;

class CpuMailbox: public Middleware
{
private:
	inline static CpuMailbox* sm_cpuMailboxPtr = nullptr;

public:

	class CpuMessageHandler
	{
	public:
		CpuMessageHandler(
			CpuMessage::MessageType type,
			CpuMessageHandlerFunc func,
			void* arg,
			const bool oneShot
		);

		bool operator==(const CpuMessageHandler& other) const;

	public:
		CpuMessage::MessageType m_type;
		CpuMessageHandlerFunc m_func;
		void* m_arg;
		const bool m_oneShot;
	};


public:
	static CpuMailbox* CreateInstance(
		const unsigned short mboxDeviceId,
		const unsigned short mboxIntrId
	);
	static CpuMailbox* GetInstance();

private:
	CpuMailbox(
		const unsigned short mboxDeviceId,
		const unsigned short mboxIntrId
	);
	virtual ~CpuMailbox();
	static void IntrHandler(CpuMailbox* cpuMailboxPtr);

public:
	void RegisterMessageHandler(const CpuMessage::MessageType type, CpuMessageHandlerFunc func, void* arg);
	void ProcessInbox();
	void ProcessOutbox();
	bool QueueMessage(CpuMessage& cpuMsg);

protected:
	virtual void Loop();
	virtual int ProcessCommand(const std::string& command);

private:
	const unsigned short m_mboxIntrId;
	CpuMessageHandlerMap m_handlerMap;
	XMbox_Config* m_mboxConfig;
	XMbox m_mbox;
	volatile bool m_hadError;
	LiteQueue<CpuMessage, CPU_MAILBOX_QUEUE_SIZE> m_sendQueue;
	LiteQueue<CpuMessage, CPU_MAILBOX_QUEUE_SIZE> m_receiveQueue;
	LiteQueue<CpuMessage, CPU_MAILBOX_QUEUE_SIZE> m_receiveQueueAux;
	LiteQueue<CpuMessage, CPU_MAILBOX_QUEUE_SIZE>* m_receiveQueuePtr;
};

// ⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢

inline CpuMailbox::CpuMessageHandler::CpuMessageHandler(
	CpuMessage::MessageType type,
	CpuMessageHandlerFunc func,
	void* arg,
	const bool oneShot
) :
	m_type(type),
	m_func(func),
	m_arg(arg),
	m_oneShot(oneShot)
{
}

inline bool CpuMailbox::CpuMessageHandler::operator==(const CpuMailbox::CpuMessageHandler& other) const
{
	return static_cast<bool>(m_type == other.m_type
		&& m_func.target<bool(*)(CpuMessage&, void*)>() == other.m_func.target<bool(*)(CpuMessage&, void*)>()
		&& m_arg == other.m_arg
		&& m_oneShot == other.m_oneShot
	);
}

namespace std
{
	template<>
	struct hash<CpuMailbox::CpuMessageHandler>
	{
		//
		inline size_t operator()(const CpuMailbox::CpuMessageHandler& handler) const
		{
			size_t hash_ = ((size_t)handler.m_type) << 10;
			hash_ += (size_t)handler.m_func.target<bool(*)(CpuMessage&, void*)>();
			hash_ += (size_t)handler.m_arg;
			return handler.m_oneShot ? ~hash_ : hash_;
		}
	};
}

// ⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢
// ⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢

#endif /* SRC_CPUMAILBOX_H_ */
