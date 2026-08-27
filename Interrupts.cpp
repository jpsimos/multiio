/*
 * Interrupts.cpp
 *
 *  Created on: Jul 10, 2025
 *      Author: jacob a psimos
 */

#include "Interrupts.h"

#include "xparameters.h"
#include "xil_exception.h"
#include "xscugic.h"
#include <algorithm>
#include <vector>
#include "Helpers.h"

using namespace Helpers;

Interrupts* Interrupts::CreateInstance()
{
	if(nullptr == Interrupts::sm_intrMgr)
	{
		Interrupts::sm_intrMgr = new Interrupts();
	}

	return Interrupts::sm_intrMgr;
}

Interrupts* Interrupts::GetInstance()
{
	if(nullptr == Interrupts::sm_intrMgr)
	{
		RUNTIME_ERROR("Must call CreateInstance first");
	}
	return Interrupts::sm_intrMgr;
}

Interrupts::Interrupts()
	: m_scu(),
	  m_intrs()
{
	int result;
	XScuGic_Config* scuConfig;

	scuConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
	result = XScuGic_CfgInitialize(&m_scu, scuConfig, scuConfig->CpuBaseAddress);

	if(XST_SUCCESS == result)
	{
		Xil_ExceptionRegisterHandler(
			XIL_EXCEPTION_ID_INT,
			reinterpret_cast<Xil_ExceptionHandler>(Interrupts::IsrProxy),
			reinterpret_cast<void*>(this)
		);

		Xil_ExceptionEnable();
	}
	else
	{
		RUNTIME_ERROR("XScuGic_CfgInitialize = %d", result);
	}
}

Interrupts::~Interrupts()
{
	for(const unsigned char intr : m_intrs)
	{
		Detach(intr);
	}
	XScuGic_Stop(&m_scu);
}

void Interrupts::IsrProxy(Interrupts* intrMgr)
{
	if(intrMgr != nullptr)
	{
		XScuGic_InterruptHandler(&(intrMgr->m_scu));
	}
}

bool Interrupts::Attached(const unsigned char intrId)
{
	for(const unsigned char intr : m_intrs)
	{
		if(intrId == intr)
		{
			return true;
		}
	}

	return false;
}

bool Interrupts::Attach(
	const unsigned char intrId,
	Xil_InterruptHandler isr,
	void* arg,
	const bool enable,
	const unsigned char priority,
	const bool highEdgeTrigger
)
{
	if(!Attached(intrId))
	{
		m_intrs.push_back(intrId);

		XScuGic_Connect(&m_scu, static_cast<u32>(intrId), isr, arg);

		XScuGic_SetPriorityTriggerType(&m_scu, static_cast<u32>(intrId), priority,
				static_cast<unsigned char>(highEdgeTrigger ? 0b11 : 0b01));

		if(enable)
		{
			XScuGic_Enable(&m_scu, static_cast<u32>(intrId));
		}

		return true;
	}

	return false;
}

bool Interrupts::Detach(const unsigned char intrId)
{
	std::vector<unsigned char>::iterator iter;

	if(Attached(intrId))
	{
		XScuGic_Disable(&m_scu, intrId);
		XScuGic_Disconnect(&m_scu, intrId);

		iter = std::find(m_intrs.begin(), m_intrs.end(), intrId);

		if(iter != m_intrs.end())
		{
			m_intrs.erase(iter);
		}

		return true;
	}
	return false;
}

bool Interrupts::Enable(const unsigned char intrId)
{
	if(Attached(intrId))
	{
		XScuGic_Enable(&m_scu, intrId);
		return true;
	}
	return false;
}

bool Interrupts::Disable(const unsigned char intrId)
{
	if(Attached(intrId))
	{
		XScuGic_Disable(&m_scu, intrId);
		return true;
	}
	return false;
}

