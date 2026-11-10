/*
 * HmiController.cpp
 *
 *  Created on: Jul 12, 2025
 *      Author: jacob
 */

#include "HmiController.h"
#include "InterruptManager.h"

HmiController* HmiController::CreateInstance(
	const unsigned short btnsGpioDeviceId,
	const unsigned char btnsGpioIntrId,
	const unsigned short ledsGpioDeviceId
)
{
	if(NULL == HmiController::s_hmi)
	{
		HmiController::s_hmi = new HmiController(
			btnsGpioDeviceId,
			btnsGpioIntrId,
			ledsGpioDeviceId
		);
	}

	return HmiController::s_hmi;
}

HmiController* HmiController::GetInstance()
{
	if(NULL == HmiController::s_hmi)
	{
		RUNTIME_ERROR("Must call CreateInstance first.");
	}

	return HmiController::s_hmi;
}

HmiController::HmiController(
	const unsigned short btnsGpioDeviceId,
	const unsigned char btnsGpioIntrId,
	const unsigned short ledsGpioDeviceId
) :
	Middleware(Middleware::AbstractType::HMI),
	m_buttonsGpioIntrId(btnsGpioIntrId),
	m_buttonsGpioConfig(XGpio_LookupConfig(btnsGpioDeviceId)),
	m_buttonsGpio(),
	m_ledsGpioConfig(XGpio_LookupConfig(ledsGpioDeviceId)),
	m_psGpioConfig(XGpioPs_LookupConfig(XPAR_XGPIOPS_0_DEVICE_ID)),
	m_psGpio(),
	m_verbose(true),
	m_lastButtonsMask(0),
	m_buttonsDebounceTimer()
{
	int result;

	result = XGpio_CfgInitialize(&m_buttonsGpio, m_buttonsGpioConfig, m_buttonsGpioConfig->BaseAddress);
	result |= XGpio_CfgInitialize(&m_ledsGpio, m_ledsGpioConfig, m_ledsGpioConfig->BaseAddress);
	result |= XGpioPs_CfgInitialize(&m_psGpio, m_psGpioConfig, m_psGpioConfig->BaseAddr);

	if(XST_SUCCESS == result)
	{
		XGpio_InterruptGlobalDisable(&m_buttonsGpio);
		XGpio_InterruptGlobalEnable(&m_buttonsGpio);
		XGpio_InterruptDisable(&m_buttonsGpio, XGPIO_IR_MASK);
		XGpio_InterruptClear(&m_buttonsGpio, XGPIO_IR_MASK);
		InterruptManager::GetInstance()->Attach(
			m_buttonsGpioIntrId,
			InterruptManager::IntrType::PL_INTR,
			reinterpret_cast<Xil_InterruptHandler>(HmiController::ButtonsIsr),
			this,
			true
		);
		XGpio_InterruptEnable(&m_buttonsGpio, XGPIO_IR_CH1_MASK);


		XGpioPs_SetDirectionPin(&m_psGpio, 0, XGPIOPS_OUTPUT);    // LED
		XGpioPs_SetOutputEnablePin(&m_psGpio, 0, 1);
		XGpioPs_SetDirectionPin(&m_psGpio, 9, XGPIOPS_INPUT);     // BTN
		XGpioPs_SetCallbackHandler(&m_psGpio, this,
			reinterpret_cast<XGpioPs_Handler>(HmiController::ButtonsIsr));
		XGpioPs_SetIntrTypePin(&m_psGpio, 9, XGPIOPS_IRQ_TYPE_EDGE_FALLING);
		XGpioPs_IntrClearPin(&m_psGpio, 9);
		InterruptManager::GetInstance()->Attach(
			XPAR_XGPIOPS_0_INTR,
			InterruptManager::IntrType::PS_INTR,
			reinterpret_cast<Xil_InterruptHandler>(XGpioPs_IntrHandler),
			&m_psGpio,
			true
		);
		XGpioPs_IntrEnablePin(&m_psGpio, 9);

		SetLeds(0b111, 0b000);
	}
	else
	{
		RUNTIME_ERROR("XGpio_CfgInitialize = %d", result);
	}

}

HmiController::~HmiController()
{
	XGpio_InterruptGlobalDisable(&m_buttonsGpio);
	XGpio_InterruptClear(&m_buttonsGpio, XGPIO_IR_CH1_MASK);
	InterruptManager::GetInstance()->Detach(m_buttonsGpioIntrId,
		InterruptManager::IntrType::PL_INTR);
}

void HmiController::ButtonsIsr(HmiController* hmi, unsigned int eventBank, unsigned int eventStatus)
{
	unsigned int status;

	if((status = XGpio_InterruptGetStatus(&(hmi->m_buttonsGpio))))
	{
		XGpio_InterruptClear(&(hmi->m_buttonsGpio), status);
	}

	if(XGpioPs_IntrGetStatusPin(&(hmi->m_psGpio), 9))
	{
		XGpioPs_IntrClearPin(&(hmi->m_psGpio), 9);
	}
}

void HmiController::SetLeds(const unsigned char mask, const unsigned char value)
{
	if(mask & 0b011)
	{
		XGpio_DiscreteSet(&m_ledsGpio, 1, (mask & 0b011));
		XGpio_DiscreteClear(&m_ledsGpio, 1, (value & mask) & 0b011);
	}

	if(mask & 0b100)
	{
		XGpioPs_WritePin(&m_psGpio, 0, !(value & 0b100));
	}
}

unsigned char HmiController::GetButtons()
{
	return static_cast<unsigned char>(XGpio_DiscreteRead(&m_buttonsGpio, 1)
		| (!!XGpioPs_ReadPin(&m_psGpio, 9) << 2));
}

void HmiController::Loop()
{
	const unsigned char buttonsMask = GetButtons();

	if(m_lastButtonsMask != buttonsMask && m_buttonsDebounceTimer >= 75000)
	{
		if(m_verbose)
		{
			std::cout << "Buttons = 0x" << std::hex << static_cast<unsigned int>(buttonsMask) << "\n";
		}

		m_lastButtonsMask = buttonsMask;
		m_buttonsDebounceTimer = 0;
	}
}

int HmiController::ProcessCommand(const std::string& command)
{
	int commandResult = COMMAND_IGNORE;
	unsigned char mask;
	unsigned char value;
	int numChars;

	if(2 == std::sscanf(command.c_str(), "leds %hhu %hhu%n", &mask, &value, &numChars)
		&& numChars == static_cast<int>(command.length()))
	{
		SetLeds(mask, value);
		commandResult = COMMAND_OK;
	}

	return commandResult;
}

