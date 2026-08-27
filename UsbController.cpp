/*
 * UsbController.cpp
 *
 *  Created on: Feb 10, 2025
 *      Author: jacob a psimos
 */

#include <iostream>
#include <string>
#include "xparameters.h"
#include "xusbps.h"
#include "xusbps_hw.h"
#include "xil_mmu.h"
#include "UsbController.h"
#include "GpioController.h"
#include "InterruptManager.h"
#include "Helpers.h"

using namespace Helpers;

UsbController* UsbController::CreateInstance(
	const unsigned short deviceId,
	const unsigned char intrId
)
{
	if(UsbController::sm_singletonPtr == nullptr)
	{
		UsbController::sm_singletonPtr = new UsbController(deviceId, intrId);
	}
	return UsbController::sm_singletonPtr;
}

UsbController* UsbController::GetInstance()
{
	if(UsbController::sm_singletonPtr == nullptr)
	{
		RUNTIME_ERROR("Must call CreateInstance first");
	}
	return UsbController::sm_singletonPtr;
}

UsbController::UsbController(
	const unsigned short deviceId,
	const unsigned char intrId
) :
	Middleware(Middleware::AbstractType::USB_CONTROLLER, MW_PRIORITY_HIGHER),
	m_deviceId(deviceId),
	m_intrId(intrId),
	m_usbConfig(XUsbPs_LookupConfig(deviceId)),
	m_usb()
{
	s32 result;

	GpioController::GetInstance()->SetPinMode(USB0_PHY_RSTN_PIN, PS_GPIO_OUT, PS_GPIO_LOW);
	usleep(10000);
	GpioController::GetInstance()->SetPin(USB0_PHY_RSTN_PIN);
	usleep(10000);

	result = XUsbPs_CfgInitialize(&m_usb, m_usbConfig, m_usbConfig->BaseAddress);

	if(XST_SUCCESS != result)
	{
		RUNTIME_ERROR("XUsbPs_CfgInitialize = %d", result);
	}

	result = XUsbPs_IntrSetHandler(
		&m_usb,
		reinterpret_cast<XUsbPs_IntrHandlerFunc>(UsbController::UsbIsr),
		this,
		XUSBPS_IXR_UE_MASK | XUSBPS_IXR_TI0_MASK
	);

	if(XST_SUCCESS != result)
	{
		RUNTIME_ERROR("IntrSetHandler = %d", result);
	}

	InterruptManager::GetInstance()->Attach(m_intrId,
		reinterpret_cast<Xil_InterruptHandler>(XUsbPs_IntrHandler), &m_usb, true, 0xA0, false);

	// UsbStorage(&m_usb, m_usbConfig);
}

UsbController::~UsbController()
{
	XUsbPs_Stop(&m_usb);
	XUsbPs_IntrDisable(&m_usb, XUSBPS_IXR_ALL);
	InterruptManager::GetInstance()->Detach(m_intrId);
}

void UsbController::UsbIsr(UsbController* usbControllerPtr, unsigned int intrMask)
{
	if(intrMask & XUSBPS_IXR_TI0_MASK)
	{
		nop();
	}
}

void UsbController::Loop()
{

}

int UsbController::ProcessCommand(const std::string& command)
{
	int commandResult = COMMAND_IGNORE;
	int err;
	bool ok;
	unsigned char val;
	unsigned char reg;
	unsigned short vendorId;
	unsigned short productId;

	if(Text::StringBeginsWith(command, "usb "))
	{
		commandResult = COMMAND_ERROR;

		if(command == "usb test")
		{
			ok = UlpiReadTransceiverId(&productId, &vendorId);

			if(ok)
			{
				std::cout << Text::Stringf("Usb Transceiver VID=%04x, PID=%04x\n", vendorId, productId);

				for(unsigned int i = 0; i < 0xFF && ok; i++)
				{
					ok = UlpiWriteScratchRegister(i);

					if(ok)
					{
						ok = UlpiReadScratchRegister(&val);
					}

					if(ok)
					{
						ok = static_cast<bool>(val == i);
					}
				}
			}

			if(ok)
			{
				commandResult = COMMAND_OK;
			}
			else
			{
				std::cout << "Usb Test Error\n";
				commandResult = COMMAND_ERROR;
			}
		}
	}

	return commandResult;
}

bool UsbController::UlpiReadTransceiverId(unsigned short* productIdPtr, unsigned short* vendorIdPtr)
{
	unsigned short vendorId = 0;
	unsigned short productId = 0;
	unsigned char val = 0;
	int err;

	err = UlpiRead(1, &val);
	vendorId = (static_cast<unsigned short>(val) << 8);
	val = 0;
	err |= UlpiRead(0, &val);
	vendorId |= (static_cast<unsigned short>(val) & 0xFF);
	val = 0;
	err |= UlpiRead(3, &val);
	productId = (static_cast<unsigned short>(val) << 8);
	val = 0;
	err |= UlpiRead(2, &val);
	productId |= (static_cast<unsigned short>(val) & 0xFF);

	if(nullptr != productIdPtr)
	{
		*productIdPtr = productId;
	}

	if(nullptr != vendorIdPtr)
	{
		*vendorIdPtr = vendorId;
	}

	return static_cast<bool>(ULPI_OK == err);
}

bool UsbController::UlpiWriteScratchRegister(const unsigned char value)
{
	int err = UlpiWrite(0x16, value);
	return static_cast<bool>(ULPI_OK == err);
}

bool UsbController::UlpiReadScratchRegister(unsigned char* value)
{
	int err = UlpiRead(0x16, value);
	return static_cast<bool>(ULPI_OK == err);
}

int UsbController::UlpiWait(unsigned int mask)
{
	Time::Stopwatch timeout;
	u32 reg;

	do
	{
		reg = XUsbPs_ReadReg(m_usbConfig->BaseAddress, XUSBPS_ULPIVIEW_OFFSET);

		if(0 == (reg & mask))
		{
			return ULPI_OK;
		}

		Yield(500);
	}
	while(timeout < 10000);

	return ULPI_ERROR;
}

int UsbController::UlpiWakeup()
{
	int err = ULPI_OK;
	u32 reg = XUsbPs_ReadReg(m_usbConfig->BaseAddress, XUSBPS_ULPIVIEW_OFFSET);

	if(~reg & XUSBPS_ULPIVIEW_SS_MASK)
	{
		XUsbPs_WriteReg(m_usbConfig->BaseAddress, XUSBPS_ULPIVIEW_OFFSET, XUSBPS_ULPIVIEW_WU_MASK);
		err = UlpiWait(XUSBPS_ULPIVIEW_WU_MASK);
	}

	return err;
}

int UsbController::UlpiRequest(unsigned int value)
{
	int err = UlpiWakeup();

	if(ULPI_OK == err)
	{
		XUsbPs_WriteReg(m_usbConfig->BaseAddress, XUSBPS_ULPIVIEW_OFFSET, (u32)value);
		err = UlpiWait(XUSBPS_ULPIVIEW_RUN_MASK);
	}

	return err;
}

int UsbController::UlpiWrite(unsigned char reg, unsigned char value)
{
	const u32 portNo = 0;
	int err = ULPI_OK;
	unsigned int request = XUSBPS_ULPIVIEW_RUN_MASK | XUSBPS_ULPIVIEW_RW_MASK
		| (static_cast<unsigned int>(reg) << 16)
		| static_cast<unsigned int>(value)
		| ((portNo & 0x7) << 24);

	err = UlpiRequest(request);
	return err;
}

int UsbController::UlpiBitModify(unsigned char reg, unsigned char mask, unsigned char value)
{
	int err = ULPI_OK;
	unsigned char value_ = 0;

	err = UlpiRead(reg, &value_);

	if(ULPI_OK == err)
	{
		value_ &= ~mask;
		value_ |= (value & mask);
		err = UlpiWrite(reg, value_);
	}

	return err;
}

int UsbController::UlpiRead(unsigned char reg, unsigned char* valuePtr)
{
	const u32 portNo = 0;
	int err;
	u32 val;

	val = XUSBPS_ULPIVIEW_RUN_MASK | (static_cast<unsigned int>(reg) << 16)
			| ((portNo & 0x7) << 24);

	err = UlpiRequest(val);

	if(ULPI_OK == err)
	{
		val = XUsbPs_ReadReg(m_usbConfig->BaseAddress, XUSBPS_ULPIVIEW_OFFSET);
		if(NULL != valuePtr)
		{
			val = (val >> 8) & 0xFF;
			*valuePtr = static_cast<unsigned char>(val);
		}
	}

	return err;
}

