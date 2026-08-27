/*
 * Buttons.h
 *
 *  Created on: Jun 7, 2026
 *      Author: jpsim
 */

#ifndef SRC_BUTTONS_H_
#define SRC_BUTTONS_H_

#include "Middleware.h"
#include "GpioController.h"
#include "Interrupts.h"

class Buttons: public Middleware
{
private:
	inline static Buttons* sm_singletonPtr = nullptr;

public:
	static Buttons* CreateInstance();
	static Buttons* GetInstance();

private:
	Buttons();
	virtual ~Buttons();
	static void GpioIsr(Buttons* buttonsPtr);

public:
	virtual void Loop();

private:
	bool m_psButtonStatus;
	bool m_plButton0Status;
	bool m_plButton1Status;

};

#endif /* SRC_BUTTONS_H_ */
