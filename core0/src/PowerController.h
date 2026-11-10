/*
 * PowerController.h
 *
 *  Created on: Jun 15, 2025
 *      Author: jacob a psimos
 */

#ifndef SRC_POWERCONTROLLER_H_
#define SRC_POWERCONTROLLER_H_

#include "xgpio.h"
#include "xtmrctr.h"
#include "Middleware.h"
#include "Helpers.h"
#include <array>

using namespace Helpers;

#define PWRCTL_NUM_ZC      2
#define PWRCTL_ZC_POLAR    1         // Active high
#define PWRCTL_ZC_1        0b0001
#define PWRCTL_ZC_2        0b0010
#define PWRCTL_ZC_MASK     0b0011
#define PWRCTL_PSN_1       0b0001
#define PWRCTL_PSN_2       0b0010
#define PWRCTL_PSN_MASK    0b0011
#define PWRCTL_MOSFET_1    0b0100
#define PWRCTL_MOSFET_MASK 0b0100
#define PWRCTL_RELAY_1     0b1000
#define PWRCTL_RELAY_MASK  0b1000

#define PWRCTL_IN_CHAN        2
#define PWRCTL_IN_CHAN_MASK   XGPIO_IR_CH2_MASK
#define PWRCTL_OUT_CHAN       1
#define PWRCTL_OUT_CHAN_MASK  XGPIO_IR_CH1_MASK

class PowerController: public Middleware
{
private:
	inline static PowerController* sm_pwrCtrlPtr = NULL;

public:
	static PowerController* CreateInstance(
		const unsigned short gpioDeviceId,
		const unsigned char gpioIntrId,
		const unsigned short axiTimerDeviceId,
		const unsigned char axiTimerIntrId
	);
	static PowerController* GetInstance();

private:
	PowerController(
		const unsigned short gpioDeviceId,
		const unsigned char gpioIntrId,
		const unsigned short axiTimerDeviceId,
		const unsigned char axiTimerIntrId
	);
	virtual ~PowerController();
	void ScheduleTimer(const unsigned char timer, const unsigned int micros, const bool start);
	static void GpioIsr(PowerController* pwrCtrl);
	static void TimerIsr(PowerController* pwrCtrl, u8 timerNum);


protected:
	virtual void Loop();
	virtual int ProcessCommand(const std::string& command);

private:
	const unsigned char m_gpioIntrId;
	const unsigned char m_axiTimerIntrId;
	XGpio_Config* m_gpioConfig;
	XGpio m_gpio;
	XTmrCtr m_axiTimer;
	bool m_zcHadAtleastOnePeriod[PWRCTL_NUM_ZC];
	volatile bool m_psnEnable[PWRCTL_NUM_ZC];
	volatile unsigned int m_zcRisingEdges[PWRCTL_NUM_ZC];
	volatile unsigned int m_zcFallingEdges[PWRCTL_NUM_ZC];
	volatile unsigned int m_psnCount[PWRCTL_NUM_ZC];
};

#endif /* SRC_POWERCONTROLLER_H_ */
