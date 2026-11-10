/*
 * InterruptManager.cpp
 *
 *  Created on: Jul 10, 2025
 *      Author: jacob a psimos
 */

#include "InterruptManager.h"
#include "xparameters.h"
#include "xil_exception.h"
#include "xintc.h"
#include "xscugic.h"
#include <algorithm>
#include <vector>
#include "Helpers.h"

using namespace Helpers;

InterruptManager* InterruptManager::CreateInstance(
	const unsigned short intcDeviceId,
	const unsigned char axiIntrId
)
{
	if(NULL == InterruptManager::sm_intrMgr)
	{
		InterruptManager::sm_intrMgr = new InterruptManager(intcDeviceId, axiIntrId);
	}
	return InterruptManager::sm_intrMgr;
}

InterruptManager* InterruptManager::GetInstance()
{
	if(NULL == InterruptManager::sm_intrMgr)
	{
		RUNTIME_ERROR("Must call CreateInstance first");
	}
	return InterruptManager::sm_intrMgr;
}

InterruptManager::InterruptManager(
	const unsigned short intcDeviceId,
	const unsigned char axiIntrId
)
	: m_intcIntrId(axiIntrId),
	  m_scu(),
	  m_intc(),
	  m_intrs()
{
	int result;
	XScuGic_Config* scuConfig;

	scuConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
	result = XScuGic_CfgInitialize(&m_scu, scuConfig, scuConfig->CpuBaseAddress);

	if(XST_SUCCESS == result)
	{
		Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			reinterpret_cast<Xil_ExceptionHandler>(XScuGic_InterruptHandler), &m_scu);

		result = XIntc_Initialize(&m_intc, intcDeviceId);

		// XIntc_SetOptions(&m_intc, XIN_SVC_ALL_ISRS_OPTION);

		if(XST_SUCCESS == result)
		{
			result = XIntc_Start(&m_intc, XIN_REAL_MODE);

			if(XST_SUCCESS == result)
			{
				Xil_ExceptionEnable();

				Attach(axiIntrId, InterruptManager::IntrType::PS_INTR,
					reinterpret_cast<Xil_InterruptHandler>(InterruptManager::FabricIsr), this, true);
			}
			else
			{
				RUNTIME_ERROR("XIntc_Start = %d", result);
			}
		}
		else
		{
			RUNTIME_ERROR("XIntc_Initialize = %d", result);
		}
	}
	else
	{
		RUNTIME_ERROR("XScuGic_CfgInitialize = %d", result);
	}
}

InterruptManager::~InterruptManager()
{
	for(std::pair<InterruptManager::IntrType, unsigned char>& intr : m_intrs)
	{
		Detach(intr.second, intr.first);
	}
	XIntc_Stop(&m_intc);
	XScuGic_Stop(&m_scu);
}

void InterruptManager::FabricIsr(InterruptManager* intrMgr)
{
	XIntc_InterruptHandler(&(intrMgr->m_intc));
}

bool InterruptManager::Attach(
	const unsigned char intrId,
	const InterruptManager::IntrType intrType,
	Xil_InterruptHandler isr,
	void* arg,
	const bool enable
)
{
	std::vector<std::pair<InterruptManager::IntrType, unsigned char>>::iterator iter = m_intrs.begin();

	while(m_intrs.end() != iter)
	{
		if(intrType == iter->first && intrId == iter->second)
		{
			return false;
		}

		iter++;
	}

	if(m_intrs.end() == iter)
	{
		m_intrs.push_back({ intrType, intrId });

		if(InterruptManager::IntrType::PS_INTR == intrType
			|| InterruptManager::IntrType::SW_INTR == intrType)
		{
			XScuGic_Connect(&m_scu, static_cast<u32>(intrId), isr, arg);

			if(reinterpret_cast<Xil_InterruptHandler>(InterruptManager::FabricIsr) == isr)
			{
				XScuGic_SetPriorityTriggerType(&m_scu, static_cast<u32>(intrId), 0xA0 / 2, 0x03);
			}
			else
			{
				XScuGic_SetPriorityTriggerType(&m_scu, static_cast<u32>(intrId), 0xA0, 0x01);
			}

			if(enable)
			{
				XScuGic_Enable(&m_scu, static_cast<u32>(intrId));
			}
		}
		else if(InterruptManager::IntrType::PL_INTR == intrType)
		{
			XIntc_Connect(&m_intc, intrId, reinterpret_cast<XInterruptHandler>(isr), arg);

			if(enable)
			{
				XIntc_Enable(&m_intc, intrId);
			}
		}
	}

	return true;
}

bool InterruptManager::Detach(const unsigned char intrId, const InterruptManager::IntrType intrType)
{
	std::vector<std::pair<InterruptManager::IntrType, unsigned char>>::iterator iter = m_intrs.begin();

	while(m_intrs.end() != iter)
	{
		if(intrType == iter->first && intrId == iter->second)
		{
			m_intrs.erase(iter);

			if(InterruptManager::IntrType::PS_INTR == intrType
				|| InterruptManager::IntrType::SW_INTR == intrType)
			{
				XScuGic_Disable(&m_scu, intrId);
				XScuGic_Disconnect(&m_scu, intrId);
			}
			else if(InterruptManager::IntrType::PL_INTR == intrType)
			{
				XIntc_Disable(&m_intc, intrId);
				XIntc_Disconnect(&m_intc, intrId);
			}

			return true;
		}

		iter++;
	}

	return false;
}

bool InterruptManager::Enable(const unsigned char intrId, const InterruptManager::IntrType intrType)
{
	std::vector<std::pair<InterruptManager::IntrType, unsigned char>>::iterator iter = m_intrs.begin();

	while(m_intrs.end() != iter)
	{
		if(intrType == iter->first && intrId == iter->second)
		{
			if(InterruptManager::IntrType::PS_INTR == intrType
				|| InterruptManager::IntrType::SW_INTR == intrType)
			{
				XScuGic_Enable(&m_scu, intrId);
			}
			else if(InterruptManager::IntrType::PL_INTR == intrType)
			{
				XIntc_Enable(&m_intc, intrId);
			}

			return true;
		}

		iter++;
	}

	return false;
}

bool InterruptManager::Disable(const unsigned char intrId, const InterruptManager::IntrType intrType)
{
	std::vector<std::pair<InterruptManager::IntrType, unsigned char>>::iterator iter = m_intrs.begin();

	while(m_intrs.end() != iter)
	{
		if(intrType == iter->first && intrId == iter->second)
		{
			if(InterruptManager::IntrType::PS_INTR == intrType
				|| InterruptManager::IntrType::SW_INTR == intrType)
			{
				XScuGic_Disable(&m_scu, intrId);
			}
			else if(InterruptManager::IntrType::PL_INTR == intrType)
			{
				XIntc_Disable(&m_intc, intrId);
			}

			return true;
		}

		iter++;
	}

	return false;
}

bool InterruptManager::SwInterrupt(const unsigned char intrId)
{
	if(intrId >= 0 && intrId <= 15)
	{
		XScuGic_SoftwareIntr(&m_scu, static_cast<u32>(intrId), XPAR_CPU_ID ^ 1);
		return true;
	}
	return false;
}
