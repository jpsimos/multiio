/*
 * Interrupts.h
 *
 *  Created on: Jul 10, 2025
 *      Author: jacob a psimos
 */

#ifndef SRC_INTERRUPTS_H_
#define SRC_INTERRUPTS_H_

#include "xscugic.h"
#include "xil_exception.h"
#include <vector>

class Interrupts
{
private:
	inline static Interrupts* sm_intrMgr = nullptr;

public:
	static Interrupts* CreateInstance();
	static Interrupts* GetInstance();

private:
	explicit Interrupts();
	virtual ~Interrupts();
	static void IsrProxy(Interrupts* intrMgrPtr);

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

#endif /* SRC_INTERRUPTS_H_ */
