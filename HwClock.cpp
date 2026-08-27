/*
 * HwClock.cpp
 *
 *  Created on: Jan 15, 2025
 *      Author: jacob a psimos
 */

#include <iostream>
#include <string>
#include "xparameters.h"
#include "HwClock.h"
#include "GpioController.h"
#include "Interrupts.h"

using namespace Helpers;

HwClock* HwClock::CreateInstance(
	const unsigned short iicDeviceId,
	const unsigned char iicIntrId,
	const unsigned short iicAddress
)
{
	if(nullptr == HwClock::sm_hwClockPtr)
	{
		HwClock::sm_hwClockPtr = new HwClock(
			iicDeviceId,
			iicIntrId,
			iicAddress
		);
	}
	return HwClock::sm_hwClockPtr;
}

HwClock* HwClock::GetInstance()
{
	if(nullptr == HwClock::sm_hwClockPtr)
	{
		RUNTIME_ERROR("Must call CreateInstance first.");
	}
	return HwClock::sm_hwClockPtr;
}

HwClock::HwClock(
	const unsigned short iicDeviceId,
	const unsigned char iicIntrId,
	const unsigned short iicAddress
) :
	Middleware(Middleware::AbstractType::EXT_HW_CLOCK),
	m_iicDeviceId(iicDeviceId),
	m_iicIntrId(iicIntrId),
	m_iicAddress(iicAddress),
	m_iicConfigPtr(XIicPs_LookupConfig(iicDeviceId)),
	m_clockOutFrequency(ClockOutFrequency::CLKOUT_UNKNOWN),
	m_iic()
{
	int status;

	status = XIicPs_CfgInitialize(&m_iic, m_iicConfigPtr, m_iicConfigPtr->BaseAddress);

	if(XST_SUCCESS != status)
	{
		RUNTIME_ERROR("CfgInitialize = %d", status);
	}

	GpioController::GetInstance()->SetPinMode(RTC_INTR_PIN, PS_GPIO_IN);

	XIicPs_SetStatusHandler(&m_iic, this, reinterpret_cast<XIicPs_IntrHandler>(HwClock::IicIsr));

	Interrupts::GetInstance()->Attach(m_iicIntrId,
		reinterpret_cast<Xil_InterruptHandler>(XIicPs_MasterInterruptHandler), &m_iic, true, 0xA0, false);

	status = XIicPs_SetSClk(&m_iic, 400000);

	if(XST_SUCCESS != status)
	{
		RUNTIME_ERROR("SetSClk = %d", status);
	}

	if(IsBatteryLow())
	{
		std::cout << ANSI_LIGHT_YELLOW "Warning: RTC battery is low." ANSI_RESET "\n";
	}

	if(IsStopped())
	{
		std::cout << ANSI_LIGHT_YELLOW "Warning: RTC is stopped." ANSI_RESET "\n";
	}

	Random::SeedRandom(GetEpoch());
}

HwClock::~HwClock()
{
	XIicPs_Abort(&m_iic);
	Interrupts::GetInstance()->Detach(m_iicIntrId);
}

bool HwClock::BusBusyWait(const unsigned int timeoutMicros)
{
	bool stillBusy;
	Time::Stopwatch timeout;

	do
	{
		stillBusy = XIicPs_BusIsBusy(&m_iic);

		if(stillBusy && timeoutMicros > 0)
		{
			if(timeout >= timeoutMicros)
			{
				break;
			}
		}
	}
	while(stillBusy);

	return stillBusy;
}

void HwClock::IicIsr(HwClock* hwClockPtr, unsigned int StatusEvent)
{
	if(hwClockPtr != nullptr)
	{
		if(StatusEvent & XIICPS_EVENT_COMPLETE_SEND)
		{
			nop();
		}

		if(StatusEvent & XIICPS_EVENT_COMPLETE_RECV)
		{
			nop();
		}

		if(StatusEvent & XIICPS_EVENT_SLAVE_RDY)
		{
			nop();
		}

		if(StatusEvent & XIICPS_EVENT_ERROR)
		{
			nop();
		}
	}
}

void HwClock::RtcIsr(HwClock* hwClockPtr)
{
	if(hwClockPtr != nullptr)
	{
		nop();
	}
}

int HwClock::ProcessCommand(const std::string& command)
{
	int commandResult = COMMAND_IGNORE;
	HwClock::DateTime dt;
	unsigned long epoch;
	int numChars;

	if(Text::StringBeginsWith(command, "hwclock "))
	{
		if(command == "hwclock read")
		{
			std::cout << GetDateTimeString() << "\n";
			commandResult = COMMAND_OK;
		}
		else if(command == "hwclock read epoch")
		{
			std::cout << std::dec << GetEpoch() << "\n";


			commandResult = COMMAND_OK;
		}
		else if(command == "hwclock status")
		{
			std::cout << "HadPowerLoss = " << HadPowerloss() << "\n"
				<< "IsBatteryLow = " << IsBatteryLow() << "\n"
				<< "IsStopped = " << IsStopped() << "\n";
			commandResult = COMMAND_OK;
		}
		else if(6 == std::sscanf(command.c_str(), "hwclock set %u/%u/%u %u:%u:%u%n",
			reinterpret_cast<unsigned int*>(&dt.Month),
			reinterpret_cast<unsigned int*>(&dt.Day),
			reinterpret_cast<unsigned int*>(&dt.Year),
			reinterpret_cast<unsigned int*>(&dt.Hour),
			reinterpret_cast<unsigned int*>(&dt.Min),
			reinterpret_cast<unsigned int*>(&dt.Sec),
			&numChars) && numChars == static_cast<int>(command.length()))
		{
			commandResult = SetDateTime(dt) ? COMMAND_OK : COMMAND_ERROR;
		}
		else if(std::sscanf(command.c_str(), "hwclock set %lu%n", &epoch, &numChars)
			&& numChars == static_cast<int>(command.length()))
		{
			dt = HwClock::DateTime(static_cast<uint32_t>(epoch));
			commandResult = SetDateTime(dt) ? COMMAND_OK : COMMAND_ERROR;
		}
	}

	return commandResult;
}

bool HwClock::SoftwareReset()
{
	if(ModifyRegister(0U, 0b01011000, 0b01011000))
	{
		Yield(2500);
		return true;
	}

	return false;
}

bool HwClock::HadPowerloss(const bool clearFlag)
{
	bool hadPowerLoss = false;
	bool hadError;
	unsigned char reg;

	hadError = !ReadRegister(2U, reg);

	if(!hadError)
	{
		hadPowerLoss = static_cast<bool>(reg & 0b00001000);

		if(clearFlag && hadPowerLoss)
		{
			hadError = !WriteRegister(2U, reg & 0b11110111);

			if(hadError)
			{
				DEBUG_ERROR("WriteRegister");
			}
		}
	}
	else
	{
		DEBUG_ERROR("ReadRegister");
	}

	return hadPowerLoss;
}

bool HwClock::IsBatteryLow()
{
	bool isBatteryLow = false;
	bool hadError;
	unsigned char reg;

	hadError = !ReadRegister(2U, reg);

	if(!hadError)
	{
		isBatteryLow = static_cast<bool>(reg & 0b00000100);
	}
	else
	{
		DEBUG_ERROR("ReadRegister");
	}

	return isBatteryLow;
}

bool HwClock::IsStopped()
{
	bool isStopped = false;
	bool hadError;
	unsigned char reg;

	hadError = !ReadRegister(0U, reg);

	if(!hadError)
	{
		isStopped = static_cast<bool>(reg & 0b00100000);
	}
	else
	{
		DEBUG_ERROR("ReadRegister");
	}

	return isStopped;
}

bool HwClock::SetClockOutFrequency(const ClockOutFrequency freq)
{
	return ModifyRegister(15U, 0b00111000, static_cast<unsigned int>(freq) << 3);
}

bool HwClock::SetDateTime(HwClock::DateTime& dt)
{
	return SetDateTime(
		dt.Sec,
		dt.Min,
		dt.Hour,
		dt.Day,
		dt.WeekDay,
		dt.Month,
		dt.Year
	);
}

bool HwClock::SetDateTime(
	uint32_t& seconds,
	uint32_t& minutes,
	uint32_t& hours,
	uint32_t& days,
	uint32_t& weekdays,
	uint32_t& months,
	uint32_t& years
)
{
	bool hadError = false;
	unsigned char data[7];

	data[0] = static_cast<unsigned char>(Numeric::DecToBCD(seconds));
	data[1] = static_cast<unsigned char>(Numeric::DecToBCD(minutes));
	data[2] = static_cast<unsigned char>(Numeric::DecToBCD(hours));
	data[3] = static_cast<unsigned char>(Numeric::DecToBCD(days));
	data[4] = static_cast<unsigned char>(Numeric::DecToBCD(weekdays));
	data[5] = static_cast<unsigned char>(Numeric::DecToBCD(months));
	data[6] = static_cast<unsigned char>(Numeric::DecToBCD(years >= 2000 ? years - 2000 : years));

	hadError = !WriteRegisters(3U, data, 7U);

	if(!hadError)
	{
		hadError = !WriteRegister(2U, 0U);
	}

	return !hadError;
}

bool HwClock::GetDateTime(HwClock::DateTime& dt)
{
	return GetDateTime(
		dt.Sec,
		dt.Min,
		dt.Hour,
		dt.Day,
		dt.WeekDay,
		dt.Month,
		dt.Year
	);
}

bool HwClock::GetDateTime(
	uint32_t& seconds,
	uint32_t& minutes,
	uint32_t& hours,
	uint32_t& days,
	uint32_t& weekdays,
	uint32_t& months,
	uint32_t& years
)
{
	bool hadError = false;
	unsigned char regs[7];

	regs[0] = 3;

	hadError = !ReadRegisters(regs, 7);

	if(!hadError)
	{
		seconds = static_cast<unsigned int>(Numeric::BCDToDec(regs[0] & 0b01111111));
		minutes = static_cast<unsigned int>(Numeric::BCDToDec(regs[1] & 0b01111111));
		hours = static_cast<unsigned int>(Numeric::BCDToDec(regs[2] & 0b00111111));
		days = static_cast<unsigned int>(Numeric::BCDToDec(regs[3] & 0b00111111));
		weekdays = static_cast<unsigned int>(Numeric::BCDToDec(regs[4] & 0b00000111));
		months = static_cast<unsigned int>(Numeric::BCDToDec(regs[5] & 0b00011111));
		years = 2000U + static_cast<unsigned int>(Numeric::BCDToDec(regs[6]));
	}

	return !hadError;
}

uint32_t HwClock::GetEpoch()
{
	HwClock::DateTime dt;

	GetDateTime(dt);

	return dt.ToEpoch();
}

std::string HwClock::GetDateTimeString()
{
	std::string dateTimeString;
	HwClock::DateTime dt;

	if(GetDateTime(dt))
	{
		dateTimeString = Text::Stringf("%u/%u/%u %02u:%02u:%02u  %u",
			static_cast<unsigned int>(dt.Month),
			static_cast<unsigned int>(dt.Day),
			static_cast<unsigned int>(dt.Year),
			static_cast<unsigned int>(dt.Hour),
			static_cast<unsigned int>(dt.Min),
			static_cast<unsigned int>(dt.Sec),
			static_cast<unsigned int>(dt.ToEpoch())
		);
	}

	return dateTimeString;
}

bool HwClock::ReadRegister(const unsigned char reg, unsigned char& val)
{
	bool hadError;
	int result;
	unsigned char buffer[2];

	buffer[0] = reg;
	buffer[1] = 0xFF;

	hadError = true;
	result = XIicPs_MasterSendPolled(&m_iic, &buffer[0], 1, m_iicAddress);

	if(XST_SUCCESS == result)
	{
		result = XIicPs_MasterRecvPolled(&m_iic, &buffer[1], 1, m_iicAddress);

		if(XST_SUCCESS == result)
		{
			hadError = false;
		}
	}

	if(!hadError && BusBusyWait(1000000))
	{
		DEBUG_ERROR("Bus is busy.");
	}

	return !hadError;
}

bool HwClock::ReadRegisters(unsigned char regs[], const unsigned int numRegs)
{
	bool hadError;
	int result;

	hadError = true;
	result = XIicPs_MasterSendPolled(&m_iic, &regs[0], 1, m_iicAddress);

	if(XST_SUCCESS == result)
	{
		result = XIicPs_MasterRecvPolled(&m_iic, &regs[0], (s32)numRegs, m_iicAddress);

		if(XST_SUCCESS == result)
		{
			hadError = false;
		}
	}

	if(!hadError && BusBusyWait(1000000))
	{
		DEBUG_ERROR("Bus is busy.");
	}

	return !hadError;
}

bool HwClock::WriteRegister(const unsigned char reg, const unsigned char data)
{
	bool hadError;
	unsigned char buffer[2];
	int result;

	buffer[0] = reg;
	buffer[1] = data;

	hadError = true;
	result = XIicPs_MasterSendPolled(&m_iic, &buffer[0], 2, m_iicAddress);

	if(XST_SUCCESS == result)
	{
		hadError = false;
	}

	if(!hadError && BusBusyWait(1000000))
	{
		DEBUG_ERROR("Bus is busy.");
	}

	return !hadError;
}

bool HwClock::WriteRegisters(const unsigned char reg, const unsigned char data[], const unsigned int numRegs)
{
	bool hadError;
	int result;
	unsigned char* buffer;

	buffer = new unsigned char[1 + numRegs];
	buffer[0] = reg;
	std::memcpy(&buffer[1], &data[0], numRegs);

	hadError = true;
	result = XIicPs_MasterSendPolled(&m_iic, &buffer[0], 1 + numRegs, m_iicAddress);

	if(XST_SUCCESS == result)
	{
		hadError = false;
	}

	if(!hadError && BusBusyWait(1000000))
	{
		DEBUG_ERROR("Bus is busy.");
	}

	delete[] buffer;

	return !hadError;
}

bool HwClock::ModifyRegister(const unsigned char reg, unsigned char mask, unsigned char data)
{
	unsigned char val;

	if(ReadRegister(reg, val))
	{
		return WriteRegister(reg, (val & ~mask) | (data & mask));
	}

	return false;
}
