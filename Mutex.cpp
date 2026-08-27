/*
 * Mutex.cpp
 *
 *  Created on: Jul 14, 2026
 *      Author: Jacob A. Psimos
 */

#include "Mutex.h"

Mutex* Mutex::CreateInstance(const unsigned short deviceId)
{
	if(Mutex::sm_singletonMutexPtr == nullptr)
	{
		Mutex::sm_singletonMutexPtr = new Mutex(deviceId);
	}
	return Mutex::sm_singletonMutexPtr;
}

Mutex* Mutex::GetInstance()
{
	return Mutex::sm_singletonMutexPtr;
}

Mutex::Mutex(const unsigned short deviceId)
	: Middleware(Middleware::AbstractType::MUTEX),
	  m_mutexConfigPtr(XMutex_LookupConfig(deviceId)),
	  m_mutex()
{
	XMutex_CfgInitialize(&m_mutex, m_mutexConfigPtr, m_mutexConfigPtr->BaseAddress);
}

void Mutex::GetStatus(const unsigned char mutexNo, bool* ptrOwned, bool* ptrLocked)
{
	u32 owner;
	u32 locked;

	XMutex_GetStatus(&m_mutex, mutexNo, &locked, &owner);

	if(ptrOwned != nullptr)
	{
		*ptrOwned = static_cast<bool>(owner == XPAR_CPU_ID);
	}

	if(ptrLocked != nullptr)
	{
		*ptrLocked = static_cast<bool>(locked);
	}
}

void Mutex::Lock(const unsigned char mutexNum)
{
	XMutex_Lock(&m_mutex, mutexNum);
}

bool Mutex::TryLock(const unsigned char mutexNum)
{
	return static_cast<bool>(XMutex_Trylock(&m_mutex, mutexNum));
}

void Mutex::Unlock(const unsigned char mutexNum)
{
	XMutex_Unlock(&m_mutex, mutexNum);
}
