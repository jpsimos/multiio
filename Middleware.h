/*
 * Middleware.h
 * Author Jacob A Psimos
 */

#ifndef __MIDDLEWARE_HPP
#define __MIDDLEWARE_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "Helpers.h"

// Command Results
#define COMMAND_IGNORE      0            // Must be zero. Do not change.
#define COMMAND_OK          1
#define COMMAND_LAST        2
#define COMMAND_ERROR       4

// Performance Measuring
#define MW_PERF_NOT_INTERESTING 0
#define MW_PERF_NEW_WORST 1
#define MW_PERF_NEW_BEST 2

// Nested Priority
#define MW_PRIORITY_LOW 0
#define MW_PRIORITY_NORMAL 1
#define MW_PRIORITY_HIGHER 2
#define MW_PRIORITY_HIGH 3

using namespace Helpers;

class Middleware
{
public:
	enum class AbstractType : int
	{
		NONE,
		BOARD_CONTROLLER,
		BOARD_CONTROLLER_COMM,
		NETWORK_CONTROLLER,
		SD_CARD_CONTROLLER,
		QSPI_PS_FLASH_CONTROLLER,
		EXT_HW_CLOCK,
		WIFI_CONTROLLER,
		HW_CLOCK_COMM,
		XADC,
		TERMINAL,
		HEADLESS_TERMINAL,
		CAN_CONTROLLER,
		CPU_MAILBOX,
		USB_CONTROLLER,
		TCP_APP,
		MULTIMETER,
		MULTIMETER_COMM,
		EXTERNAL_ADC,
		FFT,
		ZC_DIMMER,
		MAGNETOMETER,
		HMI,
		NETWORK_MANAGER,
		PS_GPIO_CONTROLLER,
		MUTEX,
		BUTTONS,
		LEDS,
		MAILBOX,
		MAX_VALUE
	};

public:
	static void LoopRegisteredMiddlewares();
	static int ProcessCommandRegisteredMiddlewares(const std::string& command);
	void Yield(unsigned int timeoutMicros = 0U);

protected:
	explicit Middleware(
		const Middleware::AbstractType abstractType,
		const int nestedPriority = MW_PRIORITY_NORMAL
	);
	~Middleware() = default;

public:
	Middleware::AbstractType GetAbstractType() const;
	void Register();
	void Unregister();
	virtual void Loop();
	virtual int ProcessCommand(const std::string& command);

protected:
	const Middleware::AbstractType m_abstractType;
	int m_nestedPriority;
	bool m_excludeFromLoop;
	int m_lastCommandResult;
};
#endif
