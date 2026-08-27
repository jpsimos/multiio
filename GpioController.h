/*
 * GpioController.h
 *
 *  Created on: Mar 15, 2026
 *      Author: Jacob A. Psimos
 */

#ifndef SRC_GPIOCONTROLLER_H_
#define SRC_GPIOCONTROLLER_H_

#include "xgpiops.h"
#include "Middleware.h"
#include <vector>
#include <algorithm>
#include "Interrupts.h"

#define PS_GPIO_T    ~0
#define PS_GPIO_IN   0
#define PS_GPIO_OUT  1
#define PS_GPIO_OUTPUT_ENABLE 1
#define PS_GPIO_OUTPUT_DISABLE 0
#define PS_GPIO_HIGH  1
#define PS_GPIO_LOW   0
#define EMIO_OFFSET 54

//todo fix hdl
#define PS_BUTTON_PIN            9
#define PS_LED_PIN               0
#define USB0_PHY_RSTN_PIN        46
#define PL_BUTTON0_PIN        ((EMIO_OFFSET) + 0)
#define PL_BUTTON1_PIN        ((EMIO_OFFSET) + 1)
#define PL_LED0_PIN           ((EMIO_OFFSET) + 2)
#define PL_LED1_PIN           ((EMIO_OFFSET) + 3)
#define GEM0_PHY_RSTN_PIN     ((EMIO_OFFSET) + 4)
#define RTC_INTR_PIN          ((EMIO_OFFSET) + 5)

// Typedefs
typedef void (*GpioIsr)(void* arg);

class GpioController: public Middleware
{
public:
	inline static GpioController* sm_singletonPtr = nullptr;

	static GpioController* CreateInstance(
		const unsigned short deviceId,
		const unsigned char intrId
	);
	static GpioController* GetInstance();

private:
	GpioController(const unsigned short deviceId, const unsigned char intrId);
	virtual ~GpioController();
	static void GpioIsr(GpioController* gpioControllerPtr, unsigned int bank, unsigned int status);

public:
	void AddIsrHandler(void(*gpioIsr)(void*), void* arg);
	void RemoveIsrHandler(void(*gpioIsr)(void*));
	void SetPinMode(const unsigned char pin, const unsigned char mode, const unsigned char value = PS_GPIO_T);
	unsigned char GetPinMode(const unsigned char pin);
	void SetPin(const unsigned char pin);
	void ClearPin(const unsigned char pin);
	unsigned char ReadPin(const unsigned char pin);

private:
	const unsigned short m_deviceId;
	const unsigned char m_intrId;
	XGpioPs_Config* m_gpioConfigPtr;
	XGpioPs m_gpio;
	std::vector<std::pair<void(*)(void*), void*>> m_gpioIsrs;
};

#endif /* SRC_GPIOCONTROLLER_H_ */
