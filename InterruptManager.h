/*
 * InterruptManager.h
 *
 *  Created on: Jul 10, 2025
 *      Author: jacob a psimos
 */

#ifndef SRC_INTERRUPTMANAGER_H_
#define SRC_INTERRUPTMANAGER_H_

#include "xscugic.h"
#include "xil_exception.h"
#include <vector>

class InterruptManager
{
private:
	inline static InterruptManager* sm_intrMgr = nullptr;

public:
	static InterruptManager* CreateInstance();
	static InterruptManager* GetInstance();

private:
	explicit InterruptManager();
	virtual ~InterruptManager();
	static void IsrProxy(InterruptManager* intrMgrPtr);

public:
	bool Attached(const unsigned char intrId);
	bool Attach(
		const unsigned char intrId,
		Xil_InterruptHandler isr,
		void* arg,
		const bool enable,
		const unsigned char priority,
		const bool highEdgeTrigger
	);
	bool Detach(const unsigned char intrId);
	bool Enable(const unsigned char intrId);
	bool Disable(const unsigned char intrId);

private:
	XScuGic m_scu;
	std::vector<unsigned char> m_intrs;

};

#endif /* SRC_INTERRUPTMANAGER_H_ */
