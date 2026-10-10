/*
 * HwClock.h
 *
 *  Created on: Jan 15, 2025
 *      Author: jacob a psimos
 */

#ifndef SRC_HWCLOCK_H_
#define SRC_HWCLOCK_H_

#include <string>
#include "Middleware.h"
#include "xiic.h"
#include "xgpio.h"
#include "Helpers.h"

using namespace Helpers;

class CpuMessage;

class HwClock : public Middleware
{
private:
	inline static HwClock* sm_hwClockPtr = nullptr;

public:
	struct HwClock_DT
	{
		uint32_t Year;       /**< Year */
		uint32_t Month;      /**< Month */
		uint32_t Day;        /**< Day */
		uint32_t Hour;       /**< Hour */
		uint32_t Min;        /**< Minute */
		uint32_t Sec;        /**< Second */
		uint32_t WeekDay;    /**< Week Day */

		HwClock_DT();
		HwClock_DT(uint32_t epoch);
		uint32_t ToEpoch();
	};

	enum class ClockOutFrequency : unsigned int
	{
		CLKOUT_32KHZ = 0b000,
		CLKOUT_16KHZ = 0b001,
		CLKOUT_8KHZ = 0b010,
		CLKOUT_4KHZ = 0b011,
		CLKOUT_1KHZ = 0b100,
		CLKOUT_32HZ = 0b101,
		CLKOUT_1HZ = 0b110,
		CLKOUT_DISABLE = 0b111
	};

public:
	static HwClock* CreateInstance(
		const unsigned short iicDeviceId,
		const unsigned char iicIntrId,
		const unsigned short rtcGpioDeviceId,
		const unsigned char rtcIntrId,
		const unsigned int iicAddress
	);

	static HwClock* GetInstance();

private:
	HwClock(
		const unsigned short iicDeviceId,
		const unsigned char iicIntrId,
		const unsigned short rtcGpioDeviceId,
		const unsigned char rtcIntrId,
		const unsigned int iicAddress
	);
	virtual ~HwClock();
	static void HwClockIsr(HwClock* hwClockPtr);
	static bool HandleReadClockCommand(CpuMessage& msg, void* arg);

public:
	virtual int ProcessCommand(const std::string& command);
	bool SoftwareReset();
	bool HadPowerloss(const bool clearFlag = false);
	bool IsBatteryLow();
	bool IsStopped();
	bool SetClockOutFrequency(const ClockOutFrequency freq);
	bool SetDateTime(HwClock::HwClock_DT& dt);
	bool SetDateTime(
		uint32_t& seconds,
		uint32_t& minutes,
		uint32_t& hours,
		uint32_t& days,
		uint32_t& weekdays,
		uint32_t& months,
		uint32_t& years
	);
	bool GetDateTime(HwClock::HwClock_DT& dt);
	bool GetDateTime(
		uint32_t& seconds,
		uint32_t& minutes,
		uint32_t& hours,
		uint32_t& days,
		uint32_t& weekdays,
		uint32_t& months,
		uint32_t& years
	);
	uint32_t GetEpoch();
	std::string GetDateTimeString();
	bool ReadRegister(const unsigned char reg, unsigned char& val);
	bool ReadRegisters(unsigned char regs[], const unsigned int numRegs);
	bool WriteRegister(const unsigned char reg, const unsigned char data);
	bool WriteRegisters(const unsigned char reg, const unsigned char data[], const unsigned int numRegs);
	bool ModifyRegister(const unsigned char reg, unsigned char mask, unsigned char data);

private:
	const unsigned short m_iicDeviceId;
	const unsigned char m_iicIntrId;
	const unsigned short m_rtcGpioDeviceId;
	const unsigned char m_rtcIntrId;
	const unsigned int m_iicAddress;
	XIic_Config* m_iicConfigPtr;
	XGpio_Config* m_rtcGpioConfigPtr;
	HwClock::ClockOutFrequency m_clockOutFrequency;
	volatile unsigned int m_hwClockIsrCount;
	XIic m_iic;
	XGpio m_gpioRtc;
};

inline HwClock::HwClock_DT::HwClock_DT()
{
	Year = 0U;
	Month = 0U;
	Day = 0U;
	Hour = 0U;
	Min = 0U;
	Sec = 0U;
	WeekDay = 0U;
}

inline HwClock::HwClock_DT::HwClock_DT(uint32_t epoch)
{
	uint32_t year;
	uint32_t month;
	uint32_t daysInYear;
	uint32_t daysInMonth;

	Sec = epoch % 60;
	epoch /= 60;
	Min = epoch % 60;
	epoch /= 60;
	Hour = epoch % 24;
	epoch /= 24;

	year = 1970;

	while(1)
	{
		daysInYear = (year % 4U == 0 && (year % 100 != 0 || year % 400 == 0))
			? 366 : 365;

		if(epoch < daysInYear)
		{
			break;
		}

		epoch -= daysInYear;
		year++;
	}

	Year = year;
	month = 1;

	while(1)
	{
		daysInMonth = ISODD(month) ? 31 : (month == 2 ? 28 : 30);

		if(epoch < daysInMonth)
		{
			break;
		}

		epoch -= daysInMonth;
		month++;
	}

	Month = month;
	Day = epoch + 1;
}

inline uint32_t HwClock::HwClock_DT::ToEpoch()
{
	uint32_t Days = 0;

	if(Year < 2000)
	{
		Year += 2000;
	}

	for(uint32_t year = 1970; year < Year; year++)
	{
		Days +=
			(year % 4U == 0 && (year % 100 != 0 || year % 400 == 0))
				? 366 : 365;
	}

	for(uint32_t month = 1; month < Month; month++)
	{
		Days += ISODD(month) ? 31 : (month == 2 ? 28 : 30);
	}

	Days += Day - 1;

	return ((Days * 24 + Hour) * 60 + Min) * 60 + Sec;
}


#endif /* SRC_HWCLOCK_H_ */
