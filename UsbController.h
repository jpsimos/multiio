/*
 * UsbController.h
 *
 *  Created on: Feb 10, 2025
 *      Author: jacob a psimos
 */

#ifndef SRC_UsbCONTROLLER_H_
#define SRC_UsbCONTROLLER_H_

#include "xusbps.h"
#include "xusbps_hw.h"
#include "xgpiops.h"
#include "Helpers.h"
#include "Middleware.h"

// ⩽  USB Defines ⩾
#define USB_DMAMEM_ADDRESS               0x7900000
#define USB_DMAMEM_SIZE                  0x0100000
#define USB_NUM_ENDPOINTS                2


// ⩽  ULPI Status ⩾
#define ULPI_OK 0
#define ULPI_ERROR 1

// ⩽  Function Control Register ⩾
#define ULPI_FUNC_CTRL_REG                       0x04
#define ULPI_FUNC_CTRL_XCVR_HS                   0b00
#define ULPI_FUNC_CTRL_XCVR_FS                   0b01
#define ULPI_FUNC_CTRL_XCVR_LS                   0b10
#define ULPI_FUNC_CTRL_XCVR_FS_LS                0b11
#define ULPI_FUNC_CTRL_XCVR_SELECT(x)            ((x) & 0b11)
#define ULPI_FUNC_CTRL_TERM_SELECT               0x04
#define ULPI_FUNC_CTRL_OP_MODE_NORMAL            0b00
#define ULPI_FUNC_CTRL_OP_MODE_NON_DRIVING       0b01
#define ULPI_FUNC_CTRL_OP_MODE_DISABLE_BS_NZRI   0b10
#define ULPI_FUNC_CTRL_OP_MODE_RESERVED          0b11
#define ULPI_FUNC_CTRL_OP_MODE(o)                (((o) & 0b11) << 3)
#define ULPI_FUNC_CTRL_RESET                     0x20
#define ULPI_FUNC_CTRL_SUSPEND_M                 0x40
#define ULPI_FUNC_CTRL_RESERVED                  0x80

// ⩽  Interface Control Register ⩾
#define ULPI_IFACE_CTRL_REG                                     0x07
#define ULPI_IFACE_CTRL_6_FS_LS_SERIAL_MODE                     0x01
#define ULPI_IFACE_CTRL_3_FS_LS_SERIAL_MODE                     0x02
#define ULPI_IFACE_CTRL_CARKIT_MODE                             0x04
#define ULPI_IFACE_CTRL_CLK_DISABLE_IN_UART_OR_CARKIT_MODE      0x00
#define ULPI_IFACE_CTRL_CLK_ENABLE_IN_UART_OR_CARKIT_MODE       0x01
#define ULPI_IFACE_CTRL_CLK_SUSPEND_M(c)                        (((c) & 0b01) << 3)
#define ULPI_IFACE_CTRL_AUTO_RESUME                             0x10
#define ULPI_IFACE_CTRL_INDICATOR_COMPLEMENT                    0x20
#define ULPI_IFACE_CTRL_INDICATOR_PASS_THRU                     0x40
#define ULPI_IFACE_CTRL_DISABLE_PROTECT                         0x80

// ⩽  OTG Register ⩾
#define ULPI_OTG_CTRL_REG                  0x0A
#define ULPI_OTG_CTRL_ID_PULLUP            0x01
#define ULPI_OTG_CTRL_DP_PULLDOWN          0x02
#define ULPI_OTG_CTRL_DM_PULLDOWN          0x04
#define ULPI_OTG_CTRL_DISCHARGE_VBUS       0x08
#define ULPI_OTG_CTRL_CHARGE_VBUS          0x10
#define ULPI_OTG_CTRL_DRIVE_VBUS           0x20
#define ULPI_OTG_CTRL_DRIVE_VBUS_EXTERN    0x40
#define ULPI_OTG_CTRL_EXTERN_VBUS_INDIC    0x80

// ⩽  CarKit Control Register ⩾
#define ULPI_CARKIT_CTRL_REG                 0x19
#define ULPI_CARKIT_CTRL_POWER               0x01
#define ULPI_CARKIT_CTRL_DRIVE_ID_GND        0x02
#define ULPI_CARKIT_CTRL_ENABLE_TXD          0x04
#define ULPI_CARKIT_CTRL_ENABLE_RXD          0x08
#define ULPI_CARKIT_CTRL_ENABLE_SPK_LEFT     0x10
#define ULPI_CARKIT_CTRL_ENABLE_SPK_RIGHT    0x20
#define ULPI_CARKIT_CTRL_ENABLE_MIC          0x40
#define ULPI_CARKIT_CTRL_RESERVED            0x80

using namespace Helpers;

class UsbController: public Middleware
{
private:
	inline static UsbController* sm_singletonPtr = nullptr;

public:
	static UsbController* CreateInstance(
		const unsigned short deviceId,
		const unsigned char intrId
	);
	static UsbController* GetInstance();

private:
	UsbController(
		const unsigned short deviceId,
		const unsigned char intrId
	);
	virtual ~UsbController();

protected:
	virtual void Loop();
	virtual int ProcessCommand(const std::string& command);

private:
	static void UsbIsr(UsbController* usbControllerPtr, unsigned int intrMask);
	bool UlpiReadTransceiverId(unsigned short* productIdPtr, unsigned short* vendorIdPtr);
	bool UlpiWriteScratchRegister(const unsigned char value);
	bool UlpiReadScratchRegister(unsigned char* value);
	int UlpiWait(unsigned int mask);
	int UlpiWakeup();
	int UlpiRequest(unsigned int value);
	int UlpiWrite(unsigned char reg, unsigned char value);
	int UlpiBitModify(unsigned char reg, unsigned char mask, unsigned char value);
	int UlpiRead(unsigned char reg, unsigned char* valuePtr);

private:
	const unsigned short m_deviceId;
	const unsigned char m_intrId;
	XUsbPs_Config* m_usbConfig;
	XUsbPs m_usb;
	XGpioPs_Config* m_gpioPsConfig;
	XGpioPs m_gpioPs;
};

#endif /* SRC_UsbCONTROLLER_H_ */
