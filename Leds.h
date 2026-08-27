/*
 * Leds.h
 *
 *  Created on: Jun 7, 2026
 *      Author: jacob a psimos
 */

#ifndef SRC_LEDS_H_
#define SRC_LEDS_H_

#include "Middleware.h"
#include "GpioController.h"

class Leds: public Middleware
{
private:
	inline static Leds* sm_singletonPtr = nullptr;

public:
	static Leds* CreateInstance();
	static Leds* GetInstance();

private:
	Leds();
	virtual ~Leds() = default;
};

#endif
