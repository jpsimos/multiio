/*
 * Buttons.cpp
 *
 *  Created on: Jun 7, 2026
 *      Author: jpsim
 */

#include "Buttons.h"
#include "Leds.h"
#include "GpioController.h"

Buttons* Buttons::CreateInstance()
{
	if(nullptr == Buttons::sm_singletonPtr)
	{
		Buttons::sm_singletonPtr = new Buttons();
	}
	return Buttons::sm_singletonPtr;
}

Buttons* Buttons::GetInstance()
{
	if(nullptr == Buttons::sm_singletonPtr)
	{
		RUNTIME_ERROR("Must call CreateInstance first.");
	}
	return Buttons::sm_singletonPtr;
}

Buttons::Buttons() :
	Middleware(Middleware::AbstractType::BUTTONS),
	m_psButtonStatus(false),
	m_plButton0Status(false),
	m_plButton1Status(false)
{
	GpioController::GetInstance()->SetPinMode(PS_BUTTON_PIN, PS_GPIO_IN);
	GpioController::GetInstance()->SetPinMode(PL_BUTTON0_PIN, PS_GPIO_IN);
	GpioController::GetInstance()->SetPinMode(PL_BUTTON1_PIN, PS_GPIO_IN);
}

Buttons::~Buttons()
{
}

void Buttons::Loop()
{
	bool psButton = GpioController::GetInstance()->GetPin(PS_BUTTON_PIN);
	bool plButton0 = GpioController::GetInstance()->GetPin(PL_BUTTON0_PIN);
	bool plButton1 = GpioController::GetInstance()->GetPin(PL_BUTTON1_PIN);

	if(psButton != m_psButtonStatus)
	{
		std::cout << "PS Button\n";
		m_psButtonStatus = psButton;
	}

	if(plButton0 != m_plButton0Status)
	{
		std::cout << "PL Button 0\n";
		m_plButton0Status = plButton0;
	}

	if(plButton1 != m_plButton1Status)
	{
		std::cout << "PL Button 1\n";
		m_plButton1Status = plButton1;
	}
}
