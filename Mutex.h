/*
 * Mutex.h
 *
 *  Created on: Jul 14, 2026
 *      Author: Jacob A. Psimos
 */

#ifndef SRC_MUTEX_H_
#define SRC_MUTEX_H_

#include "Middleware.h"
#include "xmutex.h"

class Mutex: public Middleware
{
private:
	inline static Mutex* sm_singletonMutexPtr = nullptr;

public:
	static Mutex* CreateInstance(const unsigned short deviceId);
	static Mutex* GetInstance();

private:

	Mutex(const unsigned short deviceId);
	virtual ~Mutex() = default;

public:
	void GetStatus(const unsigned char mutexNo, bool* ptrOwned, bool* ptrLocked);
	void Lock(const unsigned char mutexNum);
	bool TryLock(const unsigned char mutexNum);
	void Unlock(const unsigned char mutexNum);

private:
	XMutex_Config* m_mutexConfigPtr;
	XMutex m_mutex;

};

#endif /* SRC_MUTEX_H_ */
