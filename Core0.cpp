/*
 * Core0.cpp
 * Author Jacob A. Psimos
 */

#include "xparameters.h"
#include "xil_mmu.h"
#include "Helpers.h"
#include "Mutex.h"
#include "HwClock.h"
#include "Buttons.h"
#include "Leds.h"
#include "XAdc.h"
#include <iostream>
#include <string>
#include "Interrupts.h"
#include "UartPsController.h"

using namespace Helpers;

int main(void)
{
	Interrupts::CreateInstance();

	Mutex::CreateInstance(
			XPAR_MICROBLAZE_HEIR_MUTEX_0_IF_0_DEVICE_ID
	)->Register();

	GpioController::CreateInstance(
		XPAR_XGPIOPS_0_DEVICE_ID,
		XPAR_XGPIOPS_0_INTR
	)->Register();

	Leds::CreateInstance()->Register();

	Buttons::CreateInstance()->Register();

	UartPsController::CreateInstance(
		XPAR_XUARTPS_0_DEVICE_ID,
		XPAR_XUARTPS_0_INTR
	)->Register();

	HwClock::CreateInstance(
		XPAR_XIICPS_0_DEVICE_ID,
		XPAR_XIICPS_0_INTR,
		0x68U
	)->Register();

	XAdc::CreateInstance(
		XPAR_XADCPS_0_DEVICE_ID,
		XPAR_XADCPS_INT_ID
	)->Register();

	std::cout << "Core 0 started.\n";
	std::cout << "RTC: " << HwClock::GetInstance()->GetDateTimeString() << '\n';

	do
	{
		Middleware::LoopRegisteredMiddlewares();
	}
	while(1 + 1 == 2);

	return 0;
}
