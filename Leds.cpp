/*
 * Leds.cpp
 *
 *  Created on: Jun 7, 2026
 *      Author: jpsim
 */

#include "Leds.h"
#include "GpioController.h"

Leds* Leds::CreateInstance()
{
	if(nullptr == Leds::sm_singletonPtr)
	{
		Leds::sm_singletonPtr = new Leds();
	}
	return Leds::sm_singletonPtr;
}

Leds* Leds::GetInstance()
{
	if(nullptr == Leds::sm_singletonPtr)
	{
		RUNTIME_ERROR("Must call CreateInstance first.");
	}
	return Leds::sm_singletonPtr;
}

Leds::Leds() :
	Middleware(Middleware::AbstractType::LEDS)
{
	GpioController::GetInstance()->SetPinMode(PS_LED_PIN, PS_GPIO_OUT, PS_GPIO_LOW);
	GpioController::GetInstance()->SetPinMode(PL_LED0_PIN, PS_GPIO_OUT, PS_GPIO_HIGH);
	GpioController::GetInstance()->SetPinMode(PL_LED1_PIN, PS_GPIO_OUT, PS_GPIO_LOW);
}

