/*
 * PowerController.cpp
 *
 *  Created on: Jun 15, 2025
 *      Author: jacob a psimos
 */

#include "PowerController.h"
#include "Middleware.h"
#include "Helpers.h"
#include "InterruptManager.h"
#include <cmath>
#include <math.h>

using namespace Helpers;

// ⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢⬢

PowerController* PowerController::CreateInstance(
	const unsigned short gpioDeviceId,
	const unsigned char gpioIntrId,
	const unsigned short axiTimerDeviceId,
	const unsigned char axiTimerIntrId
)
{
	if(NULL == PowerController::sm_pwrCtrlPtr)
	{
		PowerController::sm_pwrCtrlPtr = new PowerController(
			gpioDeviceId,
			gpioIntrId,
			axiTimerDeviceId,
			axiTimerIntrId
		);
	}
	return PowerController::sm_pwrCtrlPtr;
}

PowerController* PowerController::GetInstance()
{
	if(PowerController::sm_pwrCtrlPtr == NULL)
	{
		RUNTIME_ERROR("Must call CreateInstance first");
	}

	return PowerController::sm_pwrCtrlPtr;
}

PowerController::PowerController(
	const unsigned short gpioDeviceId,
	const unsigned char gpioIntrId,
	const unsigned short axiTimerDeviceId,
	const unsigned char axiTimerIntrId
) :
	Middleware(Middleware::AbstractType::ZC_DIMMER),
	m_gpioIntrId(gpioIntrId),
	m_axiTimerIntrId(axiTimerIntrId),
	m_gpioConfig(XGpio_LookupConfig(gpioDeviceId)),
	m_gpio(),
	m_axiTimer(),
	m_zcHadAtleastOnePeriod(),
	m_psnEnable(),
	m_zcRisingEdges(),
	m_zcFallingEdges(),
	m_psnCount()
{
	int result;

	for(unsigned int i = 0; i < PWRCTL_NUM_ZC; i++)
	{
		m_zcHadAtleastOnePeriod[i] = false;
		m_psnEnable[i] = false;
		m_zcRisingEdges[i] = 0;
		m_zcFallingEdges[i] = 0;
		m_psnCount[i] = 0;
	}

	result = XGpio_CfgInitialize(&m_gpio, m_gpioConfig, m_gpioConfig->BaseAddress);

	if(XST_SUCCESS != result)
	{
		RUNTIME_ERROR("XGpio_CfgInitialize = %d", result);
	}

	XGpio_InterruptGlobalDisable(&m_gpio);
	XGpio_InterruptDisable(&m_gpio, XGPIO_IR_MASK);
	XGpio_InterruptClear(&m_gpio, XGPIO_IR_MASK);
	InterruptManager::GetInstance()->Attach(m_gpioIntrId, InterruptManager::IntrType::PL_INTR,
		reinterpret_cast<Xil_InterruptHandler>(PowerController::GpioIsr), this, true);
	XGpio_InterruptEnable(&m_gpio, PWRCTL_IN_CHAN_MASK);
	XGpio_InterruptGlobalEnable(&m_gpio);

	result = XTmrCtr_Initialize(&m_axiTimer, axiTimerDeviceId);

	if(XST_SUCCESS != result)
	{
		RUNTIME_ERROR("XTmrCtr_Initialize = %d", result);
	}

	XTmrCtr_Stop(&m_axiTimer, 0);
	XTmrCtr_Stop(&m_axiTimer, 1);
	XTmrCtr_SetHandler(&m_axiTimer, reinterpret_cast<XTmrCtr_Handler>(PowerController::TimerIsr), this);
	XTmrCtr_SetOptions(&m_axiTimer, 0, XTC_INT_MODE_OPTION | XTC_DOWN_COUNT_OPTION);
	XTmrCtr_SetOptions(&m_axiTimer, 1, XTC_INT_MODE_OPTION | XTC_DOWN_COUNT_OPTION);
	InterruptManager::GetInstance()->Attach(m_axiTimerIntrId, InterruptManager::IntrType::PL_INTR,
		reinterpret_cast<Xil_InterruptHandler>(XTmrCtr_InterruptHandler), &m_axiTimer, true);
}

PowerController::~PowerController()
{
	XGpio_InterruptGlobalDisable(&m_gpio);
	XGpio_InterruptClear(&m_gpio, XGPIO_IR_MASK);
	InterruptManager::GetInstance()->Detach(m_gpioIntrId, InterruptManager::IntrType::PL_INTR);
	XTmrCtr_Stop(&m_axiTimer, 0);
	XTmrCtr_Stop(&m_axiTimer, 1);
	InterruptManager::GetInstance()->Detach(m_axiTimerIntrId, InterruptManager::IntrType::PL_INTR);
}

void PowerController::ScheduleTimer(const unsigned char timer, const unsigned int micros, const bool start)
{
	unsigned long long freq;

	freq = (static_cast<unsigned long long>(m_axiTimer.Config.SysClockFreqHz)
		* static_cast<unsigned long long>(micros)) / 1000000ULL;

	if(start)
	{
		XTmrCtr_Stop(&m_axiTimer, !!timer);
	}

	XTmrCtr_SetResetValue(
		&m_axiTimer,
		!!timer,
		static_cast<u32>(freq)
	);

	if(start)
	{
		XTmrCtr_Start(&m_axiTimer, !!timer);
	}
}

void PowerController::Loop()
{
	for(unsigned int i = 0; i < PWRCTL_NUM_ZC; i++)
	{
		if(!m_zcHadAtleastOnePeriod[i]
			&& m_zcRisingEdges[i] > 1 && m_zcFallingEdges[i] > 1
			&& m_zcFallingEdges[i] < m_zcRisingEdges[i])
		{
			m_zcHadAtleastOnePeriod[i] = true;
		}
	}
}

int PowerController::ProcessCommand(const std::string& command)
{
	int commandResult = COMMAND_IGNORE;
	unsigned int zc;
	unsigned int mask;
	unsigned int psn;

	if(Text::StringBeginsWith(command, "pwrctl "))
	{
		zc = XGpio_DiscreteRead(&m_gpio, PWRCTL_IN_CHAN) & PWRCTL_ZC_MASK;
		psn = XGpio_DiscreteRead(&m_gpio, PWRCTL_OUT_CHAN) & PWRCTL_PSN_MASK;

		if(command == "pwrctl status")
		{
			for(unsigned int i = 0; i < PWRCTL_NUM_ZC; ++i)
			{
				mask = (PWRCTL_ZC_1 << i);
				std::cout << "ZC[" << i << "] = 0x" << std::hex << (zc & mask) << "\n";
				std::cout << "PSN[" << i << "] = 0x" << std::hex << (psn & mask) << "\n";
			}

			commandResult = COMMAND_OK;
		}
	}

	return commandResult;
}

void PowerController::GpioIsr(PowerController* pwrCtrl)
{
	union
	{
		unsigned int intrStatus;
		unsigned int zcMask;
	};

	intrStatus = XGpio_InterruptGetStatus(&(pwrCtrl->m_gpio));

	if(intrStatus & PWRCTL_IN_CHAN_MASK)
	{
		XGpio_InterruptClear(&(pwrCtrl->m_gpio), PWRCTL_IN_CHAN_MASK);
	}

	zcMask = (XGpio_DiscreteRead(&(pwrCtrl->m_gpio), PWRCTL_IN_CHAN) & PWRCTL_ZC_MASK);

	for(unsigned int i = 0; i < PWRCTL_NUM_ZC; ++i)
	{
		if(zcMask & (PWRCTL_ZC_1 << i))
		{
			if(pwrCtrl->m_zcRisingEdges[i] <= pwrCtrl->m_zcFallingEdges[i])
			{
				pwrCtrl->m_zcRisingEdges[i] += 1;
			}
		}
		else
		{
			if(pwrCtrl->m_zcFallingEdges[i] <= pwrCtrl->m_zcRisingEdges[i])
			{
				pwrCtrl->m_zcFallingEdges[i] += 1;
			}
		}

		if(pwrCtrl->m_zcHadAtleastOnePeriod[i] && !pwrCtrl->m_psnEnable[i])
		{
			// pwrCtrl->ScheduleTimer(i, 2083, true);
			nop();
		}
	}

}

void PowerController::TimerIsr(PowerController* pwrCtrl, u8 timerNum)
{
	const unsigned int zcMask = XGpio_DiscreteRead(&(pwrCtrl->m_gpio), PWRCTL_IN_CHAN) & PWRCTL_ZC_MASK;

	for(unsigned int i = 0; i < PWRCTL_NUM_ZC; ++i)
	{
		if(zcMask & (PWRCTL_ZC_1 << i) && i == timerNum)
		{
			if(++(pwrCtrl->m_psnCount[i]) & 1)
			{
				pwrCtrl->ScheduleTimer(i, 8333, true);
			}


		}
	}

	XGpio_DiscreteClear(&(pwrCtrl->m_gpio), PWRCTL_OUT_CHAN, 0x01);
	XGpio_DiscreteSet(&(pwrCtrl->m_gpio), PWRCTL_OUT_CHAN, 0x01);
}
