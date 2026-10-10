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
#include "InterruptManager.h"

using namespace Helpers;

HwClock* HwClock::CreateInstance(
	const unsigned short iicDeviceId,
	const unsigned char iicIntrId,
	const unsigned short rtcGpioDeviceId,
	const unsigned char rtcIntrId,
	const unsigned int iicAddress
)
{
	if(nullptr == HwClock::sm_hwClockPtr)
	{
		HwClock::sm_hwClockPtr = new HwClock(
			iicDeviceId,
			iicIntrId,
			rtcGpioDeviceId,
			rtcIntrId,
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
	const unsigned short rtcGpioDeviceId,
	const unsigned char rtcIntrId,
	const unsigned int iicAddress
) :
	Middleware(Middleware::AbstractType::EXT_HW_CLOCK),
	m_iicDeviceId(iicDeviceId),
	m_iicIntrId(iicIntrId),
	m_rtcGpioDeviceId(rtcGpioDeviceId),
	m_rtcIntrId(rtcIntrId),
	m_iicAddress(iicAddress),
	m_iicConfigPtr(XIic_LookupConfig(iicDeviceId)),
	m_rtcGpioConfigPtr(XGpio_LookupConfig(rtcGpioDeviceId)),
	m_clockOutFrequency(ClockOutFrequency::CLKOUT_1HZ),
	m_hwClockIsrCount(0U),
	m_iic(),
	m_gpioRtc()
{
	int status;

	status = XIic_CfgInitialize(&m_iic, m_iicConfigPtr, m_iicConfigPtr->BaseAddress);
	status |= XGpio_CfgInitialize(&m_gpioRtc, m_rtcGpioConfigPtr, m_rtcGpioConfigPtr->BaseAddress);

	if(XST_SUCCESS != status)
	{
		RUNTIME_ERROR("..._CfgInitialize = %x", status);
	}

	if(IsBatteryLow())
	{
		std::cout << ANSI_LIGHT_YELLOW "Warning: RTC battery is low." ANSI_RESET "\n";
	}

	if(IsStopped())
	{
		std::cout << ANSI_LIGHT_YELLOW "Warning: RTC is not responsive yet." ANSI_RESET "\n";
	}

	SetClockOutFrequency(m_clockOutFrequency);
	XGpio_SetDataDirection(&m_gpioRtc, 1, 1);     // INPUT, ISR
	XGpio_InterruptGlobalDisable(&m_gpioRtc);
	XGpio_InterruptDisable(&m_gpioRtc, XGPIO_IR_MASK);
	InterruptManager::GetInstance()->Attach(
		m_rtcIntrId,
		reinterpret_cast<Xil_InterruptHandler>(HwClock::HwClockIsr),
		this,
		true,
		0xA0,
		true
	);
	XGpio_InterruptClear(&m_gpioRtc, XGPIO_IR_MASK);
	XGpio_InterruptEnable(&m_gpioRtc, XGPIO_IR_CH1_MASK);
	XGpio_InterruptGlobalEnable(&m_gpioRtc);

	Random::SeedRandom(GetEpoch());
}

HwClock::~HwClock()
{
	XGpio_InterruptDisable(&m_gpioRtc, XGPIO_IR_CH1_MASK);
	XGpio_InterruptGlobalDisable(&m_gpioRtc);
	InterruptManager::GetInstance()->Detach(m_rtcIntrId);
}

int HwClock::ProcessCommand(const std::string& command)
{
	int commandResult = COMMAND_IGNORE;
	HwClock::HwClock_DT dt;
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
				<< "IsStopped = " << IsStopped() << "\n"
				<< "ISRCount = " << m_hwClockIsrCount << "\n";
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
			dt = HwClock::HwClock_DT(static_cast<uint32_t>(epoch));
			commandResult = SetDateTime(dt) ? COMMAND_OK : COMMAND_ERROR;
		}
	}

	return commandResult;
}

void HwClock::HwClockIsr(HwClock* hwClockPtr)
{
	if(XGpio_InterruptGetStatus(&(hwClockPtr->m_gpioRtc)) & XGPIO_IR_CH1_MASK)
	{
		XGpio_InterruptClear(&(hwClockPtr->m_gpioRtc), XGPIO_IR_CH1_MASK);
	}

	hwClockPtr->m_hwClockIsrCount += 1U;
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

bool HwClock::SetDateTime(HwClock::HwClock_DT& dt)
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

bool HwClock::GetDateTime(HwClock::HwClock_DT& dt)
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
	HwClock::HwClock_DT dt;

	GetDateTime(dt);

	return dt.ToEpoch();
}

std::string HwClock::GetDateTimeString()
{
	std::string dateTimeString;
	HwClock::HwClock_DT dt;

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
	bool hadError = false;
	int status;
	unsigned int result;
	unsigned char buffer[2];

	status = XIic_IsIicBusy(&m_iic) ? XST_DEVICE_BUSY : XIic_Start(&m_iic);

	if(XST_SUCCESS == status)
	{
		status = XIic_SetAddress(&m_iic, XII_ADDR_TO_SEND_TYPE, m_iicAddress);

		if(XST_SUCCESS == status)
		{
			buffer[0] = reg;
			buffer[1] = 0xFFU;

			result = XIic_Send(m_iic.BaseAddress, m_iicAddress, (u8*)&buffer[0], 1U, XIIC_STOP);

			if(1U == result)
			{
				result = XIic_Recv(m_iic.BaseAddress, m_iicAddress, (u8*)&buffer[1], 1U, XIIC_STOP);

				if(1U == result)
				{
					val = buffer[1];
				}
				else
				{
					hadError = true;
					DEBUG_ERROR("XIic_Recv = %u", result);
				}
			}
			else
			{
				hadError = true;
				DEBUG_ERROR("XIic_Send = %u", result);
			}
		}
		else
		{
			DEBUG_ERROR("XIic_SetAddress = %d", status);
		}
	}
	else
	{
		hadError = true;
		DEBUG_ERROR("XIic_Start = %d", status);
	}

	if(hadError)
	{
		XIic_Reset(&m_iic);
	}

	return !hadError;
}

bool HwClock::ReadRegisters(unsigned char regs[], const unsigned int numRegs)
{
	bool hadError = false;
	unsigned int result;
	int status;

	status = XIic_IsIicBusy(&m_iic) ? XST_DEVICE_BUSY : XIic_Start(&m_iic);

	if(XST_SUCCESS == status)
	{
		status = XIic_SetAddress(&m_iic, XII_ADDR_TO_SEND_TYPE, m_iicAddress);

		if(XST_SUCCESS == status)
		{
			result = XIic_Send(m_iic.BaseAddress, m_iicAddress, (u8*)&regs[0], 1U, XIIC_STOP);

			if(1U == result)
			{
				result = XIic_Recv(m_iic.BaseAddress, m_iicAddress, (u8*)&regs[0], numRegs, XIIC_STOP);

				if(numRegs != result)
				{
					hadError = true;
					DEBUG_ERROR("XIic_Recv = %u", result);
				}
			}
			else
			{
				hadError = true;
				DEBUG_ERROR("XIic_Send = %u", result);
			}
		}
		else
		{
			hadError = true;
			DEBUG_ERROR("XIic_SetAddress = %x", status);
		}
	}
	else
	{
		hadError = true;
		DEBUG_ERROR("XIic_Start = %x", status);
	}


	if(hadError)
	{
		XIic_Reset(&m_iic);
	}

	return !hadError;
}

bool HwClock::WriteRegister(const unsigned char reg, const unsigned char data)
{
	bool hadError = false;
	unsigned char buffer[2];
	unsigned int result;
	int status;

	status = XIic_IsIicBusy(&m_iic) ? XST_DEVICE_BUSY : XIic_Start(&m_iic);

	if(XST_SUCCESS == status)
	{
		status = XIic_SetAddress(&m_iic, XII_ADDR_TO_SEND_TYPE, m_iicAddress);

		if(XST_SUCCESS == status)
		{
			buffer[0] = reg;
			buffer[1] = data;

			result = XIic_Send(m_iic.BaseAddress, m_iicAddress, (u8*)&buffer[0], 2U, XIIC_STOP);

			if(2U != result)
			{
				hadError = true;
				DEBUG_ERROR("XIic_Send = %u", result);
			}
		}
		else
		{
			hadError = true;
			DEBUG_ERROR("XIic_SetAddress = %d", status);
		}
	}
	else
	{
		hadError = true;
		DEBUG_ERROR("XIic_Start = %x", status);
	}

	if(hadError)
	{
		XIic_Reset(&m_iic);
	}

	return !hadError;
}

bool HwClock::WriteRegisters(const unsigned char reg, const unsigned char data[], const unsigned int numRegs)
{
	bool hadError = false;
	std::unique_ptr<unsigned char[]> buffer;
	int status;
	unsigned int result;

	status = XIic_IsIicBusy(&m_iic) ? XST_DEVICE_BUSY : XIic_Start(&m_iic);

	if(XST_SUCCESS == status)
	{
		status = XIic_SetAddress(&m_iic, XII_ADDR_TO_SEND_TYPE, m_iicAddress);

		if(XST_SUCCESS == status)
		{
			buffer.reset(new unsigned char[numRegs + 1U]);
			buffer.get()[0] = reg;
			std::memcpy(&(buffer.get()[1]), &data[0], numRegs);

			result = XIic_Send(m_iic.BaseAddress,
				m_iicAddress, (u8*)&buffer[0], numRegs + 1U, XIIC_STOP);

			if(numRegs + 1U != result)
			{
				hadError = true;
				DEBUG_ERROR("XIic_Send = %u", result);
			}
		}
		else
		{
			hadError = true;
			DEBUG_ERROR("XIic_SetAddress = %d", status);
		}
	}
	else
	{
		hadError = true;
		DEBUG_ERROR("XIic_Start = %x", status);
	}

	if(!hadError)
	{
		XIic_Reset(&m_iic);
	}

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
