/*
 * GpioController.cpp
 *
 *  Created on: Mar 15, 2026
 *      Author: Jacob A. Psimos
 */

#include "GpioController.h"

GpioController* GpioController::CreateInstance(
	const unsigned short deviceId,
	const unsigned char intrId
)
{
	if(GpioController::sm_singletonPtr == nullptr)
	{
		GpioController::sm_singletonPtr = new GpioController(deviceId, intrId);
	}
	return GpioController::sm_singletonPtr;
}

GpioController* GpioController::GetInstance()
{
	if(GpioController::sm_singletonPtr == nullptr)
	{
		RUNTIME_ERROR("Must call CreateInstance first.");
	}
	return GpioController::sm_singletonPtr;
}

GpioController::GpioController(const unsigned short deviceId, const unsigned char intrId)
	: Middleware(Middleware::AbstractType::PS_GPIO_CONTROLLER, MW_PRIORITY_NORMAL),
	  m_deviceId(deviceId),
	  m_intrId(intrId),
	  m_gpioConfigPtr(nullptr),
	  m_gpioIsrs()
{
	int result;

	m_gpioConfigPtr = XGpioPs_LookupConfig(m_deviceId);

	if(m_gpioConfigPtr == nullptr)
	{
		RUNTIME_ERROR("XGpioPs_LookupConfig");
	}

	result = XGpioPs_CfgInitialize(&m_gpio, m_gpioConfigPtr, m_gpioConfigPtr->BaseAddr);

	if(result != XST_SUCCESS)
	{
		RUNTIME_ERROR("XGpioPs_CfgInitialize = %d", result);
	}

	XGpioPs_SetCallbackHandler(&m_gpio, this,
		reinterpret_cast<XGpioPs_Handler>(GpioController::GpioIsr));

	Interrupts::GetInstance()->Attach(m_intrId,
		reinterpret_cast<Xil_InterruptHandler>(XGpioPs_IntrHandler), &m_gpio, true, 0xA0, false);
}

GpioController::~GpioController()
{
	for(unsigned char bank = 0; bank < XGPIOPS_MAX_BANKS; bank++)
	{
		XGpioPs_IntrDisable(&m_gpio, bank, 0xFFFFFFFF);
	}
	Interrupts::GetInstance()->Detach(m_intrId);
}

void GpioController::GpioIsr(GpioController* gpioControllerPtr, unsigned int bank, unsigned int status)
{
	if(gpioControllerPtr != nullptr)
	{
		for(std::pair<void(*)(void*), void*>& gpioIsrPair : gpioControllerPtr->m_gpioIsrs)
		{
			if(gpioIsrPair.first != nullptr)
			{
				gpioIsrPair.first(gpioIsrPair.second);
			}
		}
	}
}

void GpioController::AddIsrHandler(void(*gpioIsr)(void*), void* arg)
{
	m_gpioIsrs.push_back({gpioIsr, arg});
}

void GpioController::RemoveIsrHandler(void(*gpioIsr)(void*))
{
	auto iter = std::find_if(
		m_gpioIsrs.begin(),
		m_gpioIsrs.end(),
		[gpioIsr](const std::pair<void(*)(void*), void*>& gpioIsrPair) {
			return static_cast<bool>(gpioIsrPair.first == gpioIsr);
		}
	);

	if(iter != m_gpioIsrs.end())
	{
		m_gpioIsrs.erase(iter);
	}
}

void GpioController::SetPinMode(const unsigned char pin, const unsigned char mode, const unsigned char value)
{
	if(PS_GPIO_OUT != mode && XGpioPs_GetOutputEnablePin(&m_gpio, static_cast<u32>(pin)))
	{
		XGpioPs_SetOutputEnablePin(&m_gpio, static_cast<u32>(pin), PS_GPIO_OUTPUT_DISABLE);
	}

	XGpioPs_SetDirectionPin(&m_gpio, static_cast<u32>(pin), static_cast<u32>(mode));

	if(PS_GPIO_OUT == mode)
	{
		if(PS_GPIO_LOW == value)
		{
			ClearPin(pin);
		}
		else if(PS_GPIO_HIGH == value)
		{
			SetPin(pin);
		}
	}
}

unsigned char GpioController::GetPinMode(const unsigned char pin)
{
	return static_cast<unsigned char>(XGpioPs_GetDirectionPin(&m_gpio, static_cast<u32>(pin)));
}

void GpioController::SetPin(const unsigned char pin)
{
	if(PS_GPIO_OUT == XGpioPs_GetDirectionPin(&m_gpio, static_cast<u32>(pin)))
	{
		XGpioPs_WritePin(&m_gpio, static_cast<u32>(pin), PS_GPIO_HIGH);

		if(!XGpioPs_GetOutputEnablePin(&m_gpio, static_cast<u32>(pin)))
		{
			XGpioPs_SetOutputEnablePin(&m_gpio, static_cast<u32>(pin), PS_GPIO_OUTPUT_ENABLE);
		}
	}
}

void GpioController::ClearPin(const unsigned char pin)
{
	if(PS_GPIO_OUT == XGpioPs_GetDirectionPin(&m_gpio, static_cast<u32>(pin)))
	{
		XGpioPs_WritePin(&m_gpio, static_cast<u32>(pin), PS_GPIO_LOW);

		if(!XGpioPs_GetOutputEnablePin(&m_gpio, static_cast<u32>(pin)))
		{
			XGpioPs_SetOutputEnablePin(&m_gpio, static_cast<u32>(pin), PS_GPIO_OUTPUT_ENABLE);
		}
	}
}

unsigned char GpioController::ReadPin(const unsigned char pin)
{
	return static_cast<unsigned char>(XGpioPs_ReadPin(&m_gpio, static_cast<u32>(pin)));
}
