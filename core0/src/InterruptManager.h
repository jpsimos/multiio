/*
 * InterruptManager.h
 *
 *  Created on: Jul 10, 2025
 *      Author: jacob a psimos
 */

#ifndef SRC_INTERRUPTMANAGER_H_
#define SRC_INTERRUPTMANAGER_H_

#include "xintc.h"
#include "xscugic.h"
#include "xil_exception.h"
#include <vector>


class InterruptManager
{
private:
	inline static InterruptManager* sm_intrMgr = NULL;

public:
	enum IntrType
	{
		PS_INTR,
		PL_INTR,
		SW_INTR,
		AX_INTR
	};

public:
	static InterruptManager* CreateInstance(
		const unsigned short intcDeviceId,
		const unsigned char axiIntrId
	);
	static InterruptManager* GetInstance();

private:
	explicit InterruptManager(const unsigned short intcDeviceId, const unsigned char axiIntrId);
	virtual ~InterruptManager();
	static void FabricIsr(InterruptManager* intrMgrPtr);

public:
	bool Attach(
		const unsigned char intrId,
		const InterruptManager::IntrType intrType,
		Xil_InterruptHandler isr,
		void* arg,
		const bool enable
	);
	bool Detach(const unsigned char intrId, const InterruptManager::IntrType intrType);
	bool Enable(const unsigned char intrId, const InterruptManager::IntrType intrType);
	bool Disable(const unsigned char intrId, const InterruptManager::IntrType intrType);
	bool SwInterrupt(const unsigned char intrId);

private:
	const unsigned char m_intcIntrId;
	XScuGic m_scu;
	XIntc m_intc;
	std::vector<std::pair<InterruptManager::IntrType, unsigned char>> m_intrs;

};

#endif /* SRC_INTERRUPTMANAGER_H_ */
