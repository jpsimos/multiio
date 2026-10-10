/*
 * Core0.cpp
 * Author Jacob A. Psimos
 */

#include "xparameters.h"
#include "xil_mmu.h"
#include "Helpers.h"
#include "InterruptManager.h"
#include "HwClock.h"
#include <iostream>
#include <string>

using namespace Helpers;

void StartCpu1();
void ConfigureOcm();

int main(void)
{
	// ConfigureOcm();

	InterruptManager::CreateInstance();

	HwClock::CreateInstance(
		XPAR_AXI_IIC_RTC_DEVICE_ID,
		XPAR_FABRIC_AXI_IIC_RTC_IIC2INTC_IRPT_INTR,
		XPAR_AXI_GPIO_RTC_DEVICE_ID,
		XPAR_FABRIC_AXI_GPIO_RTC_IP2INTC_IRPT_INTR,
		0x68U
	)->Register();

	StartCpu1();

	std::cout << "Core 0 started.\n";

	do
	{
		static Time::Stopwatch lastTime;

		if(lastTime >= 1.0)
		{
			std::cout << HwClock::GetInstance()->GetDateTimeString() << '\n';
			lastTime = 0;
		}
	}
	while(1 + 1 == 2);

	return 0;
}

void StartCpu1()
{
	extern uint8_t _cpu1_ddr_start;
	extern uint8_t _cpu1_ddr_end;
	INTPTR address = reinterpret_cast<INTPTR>(&_cpu1_ddr_start);

	do
	{
		Xil_SetTlbAttributes(address, NORM_NONCACHE);
		address += 0x100000;
	}
	while(address < reinterpret_cast<INTPTR>(&_cpu1_ddr_end));

	Xil_Out32(0xFFFFFFF0, reinterpret_cast<INTPTR>(&_cpu1_ddr_start));

	dmb();
	sev();
}

void ConfigureOcm()
{
//	volatile uint32_t* SLCR_UNLOCK = (volatile uint32_t*)0xF8000008;
//	volatile uint32_t* SLCR_LOCK = (volatile uint32_t*)0xF8000004;
//	volatile uint32_t* SLCR_OCM_CFG = (volatile uint32_t*)0xF8000910;
//	volatile uint32_t* MPCORE_SCU_CONF = (volatile uint32_t*)0xF8F00004;
//	volatile uint32_t* MPCORE_FILT_START = (volatile uint32_t*)0xF8F00040;
//	volatile uint32_t* MPCORE_FILT_END = (volatile uint32_t*)0xF8F00044;
//
//	*SLCR_UNLOCK = 0xDF0D;
//	*SLCR_OCM_CFG |= 0xF;
//	*MPCORE_SCU_CONF = 0x1;
//	*MPCORE_FILT_START = 0x0;
//	*MPCORE_FILT_END = 0xFFE00000;
//	*SLCR_LOCK = 0x767B;

	extern uint8_t _ocm_ram_0_start;
	Xil_SetTlbAttributes(reinterpret_cast<INTPTR>(&_ocm_ram_0_start), 0x14DE2);

	dsb();
	isb();
}
