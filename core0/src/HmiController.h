/*
 * HmiController.h
 *
 *  Created on: Jul 12, 2025
 *      Author: jacob a psimos
 */

#ifndef SRC_HMICONTROLLER_H_
#define SRC_HMICONTROLLER_H_

#include "Middleware.h"
#include "xgpio.h"
#include "xgpiops.h"
#include "xtmrctr.h"
#include "Helpers.h"

#ifndef XGPIO_DEFS
#define XGPIO_DEFS
#define XGPIO_INPUT            (1)
#define XGPIO_OUTPUT           (0)
#define XGPIOPS_INPUT          (0)
#define XGPIOPS_OUTPUT         (1)
#endif

using namespace Helpers;

class HmiController: public Middleware
{
private:
	static inline HmiController* s_hmi = NULL;

public:
	static HmiController* CreateInstance(
		const unsigned short btnsGpioDeviceId,
		const unsigned char btnsGpioIntrId,
		const unsigned short ledsGpioDeviceId
	);
	static HmiController* GetInstance();

private:
	HmiController(
		const unsigned short btnsGpioDeviceId,
		const unsigned char btnsGpioIntrId,
		const unsigned short ledsGpioDeviceId
	);
	virtual ~HmiController();
	static void ButtonsIsr(HmiController* hmi, unsigned int eventBank, unsigned int eventStatus);

public:
	void SetLeds(const unsigned char mask, const unsigned char value);
	unsigned char GetButtons();
	virtual void Loop();
	virtual int ProcessCommand(const std::string& command);

private:
	const unsigned char m_buttonsGpioIntrId;
	XGpio_Config* m_buttonsGpioConfig;
	XGpio m_buttonsGpio;
	XGpio_Config* m_ledsGpioConfig;
	XGpio m_ledsGpio;
	XGpioPs_Config* m_psGpioConfig;
	XGpioPs m_psGpio;
	bool m_verbose;
	unsigned char m_lastButtonsMask;
	Time::Stopwatch m_buttonsDebounceTimer;
};

#endif /* SRC_HMICONTROLLER_H_ */
