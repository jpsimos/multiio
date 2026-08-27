/*
 * Core0.cpp
 * Author Jacob A. Psimos
 */

#include "xparameters.h"
#include "xil_mmu.h"
#include "Helpers.h"
#include "InterruptManager.h"
#include "HwClock.h"
#include "UsbController.h"
#include "UartController.h"
#include "GpioController.h"
#include "NetworkManager.h"
#include "Buttons.h"
#include "Leds.h"
#include "XAdc.h"
#include <iostream>
#include <string>

using namespace Helpers;

void StartCpu1();
void ConfigureMemory();
bool MapMemory(uint32_t physAddr, uint32_t virtAddr, uint32_t sizeBytes);

int main(void)
{
	ConfigureMemory();

	InterruptManager::CreateInstance();

	GpioController::CreateInstance(
		XPAR_XGPIOPS_0_DEVICE_ID,
		XPAR_XGPIOPS_0_INTR
	)->Register();

	Leds::CreateInstance()->Register();

	Buttons::CreateInstance()->Register();

	UartController::CreateInstance(
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

	NetworkManager::CreateInstance(
		XPAR_XEMACPS_0_DEVICE_ID,
		XPAR_XEMACPS_0_INTR
	)->Register();

	UsbController::CreateInstance(
		XPAR_XUSBPS_0_DEVICE_ID,
		XPAR_XUSBPS_0_INTR
	)->Register();

	StartCpu1();

	std::cout << "Core 0 started.\n";
	std::cout << "RTC: " << HwClock::GetInstance()->GetDateTimeString() << '\n';

	do
	{
		Middleware::LoopRegisteredMiddlewares();
	}
	while(1 + 1 == 2);

	return 0;
}

void StartCpu1()
{
	extern uint8_t _cpu1_ddr_start;
	Xil_Out32(0xFFFFFFF0, reinterpret_cast<INTPTR>(&_cpu1_ddr_start));

	dmb();
	sev();
}

void ConfigureMemory()
{
	extern uint8_t _ocm_ram_0_start;
	extern uint8_t _ocm_ram_0_end;
	extern uint8_t _cpu1_ddr_start;
	extern uint8_t _cpu1_ddr_end;
	extern uint8_t _dma1_ddr_start;
	extern uint8_t _dma1_ddr_end;
	extern uint8_t _dma2_ddr_start;
	extern uint8_t _dma2_ddr_end;
	extern uint8_t _dma3_ddr_start;
	extern uint8_t _dma3_ddr_end;
	extern uint8_t _dma4_ddr_start;
	extern uint8_t _dma4_ddr_end;
	extern uint8_t _queue1_ddr_start;
	extern uint8_t _queue1_ddr_end;
	extern uint8_t _queue2_ddr_start;
	extern uint8_t _queue2_ddr_end;
	extern uint8_t _queue3_ddr_start;
	extern uint8_t _queue3_ddr_end;
	extern uint8_t _queue4_ddr_start;
	extern uint8_t _queue4_ddr_end;
	extern uint8_t _vdisk_ddr_start;
	extern uint8_t _vdisk_ddr_end;

	uint8_t* ocm_ram_0_start = &_ocm_ram_0_start;
	uint8_t* ocm_ram_0_end = &_ocm_ram_0_end;
	uint8_t* cpu1_ddr_start = &_cpu1_ddr_start;
	uint8_t* cpu1_ddr_end = &_cpu1_ddr_end;
	uint8_t* dma1_ddr_start = &_dma1_ddr_start;
	uint8_t* dma1_ddr_end = &_dma1_ddr_end;
	uint8_t* dma2_ddr_start = &_dma2_ddr_start;
	uint8_t* dma2_ddr_end = &_dma2_ddr_end;
	uint8_t* dma3_ddr_start = &_dma3_ddr_start;
	uint8_t* dma3_ddr_end = &_dma3_ddr_end;
	uint8_t* dma4_ddr_start = &_dma4_ddr_start;
	uint8_t* dma4_ddr_end = &_dma4_ddr_end;
	uint8_t* queue1_ddr_start = &_queue1_ddr_start;
	uint8_t* queue1_ddr_end = &_queue1_ddr_end;
	uint8_t* queue2_ddr_start = &_queue2_ddr_start;
	uint8_t* queue2_ddr_end = &_queue2_ddr_end;
	uint8_t* queue3_ddr_start = &_queue3_ddr_start;
	uint8_t* queue3_ddr_end = &_queue3_ddr_end;
	uint8_t* queue4_ddr_start = &_queue4_ddr_start;
	uint8_t* queue4_ddr_end = &_queue4_ddr_end;
	uint8_t* vdisk_ddr_start = &_vdisk_ddr_start;
	uint8_t* vdisk_ddr_end = &_vdisk_ddr_end;

	for(uint8_t* ptr = ocm_ram_0_start; ptr < ocm_ram_0_end; ptr += 0x100000)
	{
		Xil_SetTlbAttributes(reinterpret_cast<INTPTR>(ptr), 0x14DE2);
	}

	for(uint8_t* ptr = cpu1_ddr_start; ptr < cpu1_ddr_end; ptr += 0x100000)
	{
		Xil_SetTlbAttributes(reinterpret_cast<INTPTR>(ptr), NORM_NONCACHE | SHAREABLE);
	}

	for(uint8_t* ptr = dma1_ddr_start; ptr < dma1_ddr_end; ptr += 0x100000)
	{
		Xil_SetTlbAttributes(reinterpret_cast<INTPTR>(ptr), DEVICE_MEMORY | EXECUTE_NEVER);
	}

	for(uint8_t* ptr = dma2_ddr_start; ptr < dma2_ddr_end; ptr += 0x100000)
	{
		Xil_SetTlbAttributes(reinterpret_cast<INTPTR>(ptr), DEVICE_MEMORY | EXECUTE_NEVER);
	}

	for(uint8_t* ptr = dma3_ddr_start; ptr < dma3_ddr_end; ptr += 0x100000)
	{
		Xil_SetTlbAttributes(reinterpret_cast<INTPTR>(ptr), DEVICE_MEMORY | EXECUTE_NEVER);
	}

	for(uint8_t* ptr = dma4_ddr_start; ptr < dma4_ddr_end; ptr += 0x100000)
	{
		Xil_SetTlbAttributes(reinterpret_cast<INTPTR>(ptr), DEVICE_MEMORY | EXECUTE_NEVER);
	}

	for(uint8_t* ptr = queue1_ddr_start; ptr < queue1_ddr_end; ptr += 0x100000)
	{
		Xil_SetTlbAttributes(reinterpret_cast<INTPTR>(ptr), DEVICE_MEMORY | EXECUTE_NEVER);
	}

	for(uint8_t* ptr = queue2_ddr_start; ptr < queue2_ddr_end; ptr += 0x100000)
	{
		Xil_SetTlbAttributes(reinterpret_cast<INTPTR>(ptr), DEVICE_MEMORY | EXECUTE_NEVER);
	}

	for(uint8_t* ptr = queue3_ddr_start; ptr < queue3_ddr_end; ptr += 0x100000)
	{
		Xil_SetTlbAttributes(reinterpret_cast<INTPTR>(ptr), DEVICE_MEMORY | EXECUTE_NEVER);
	}

	for(uint8_t* ptr = queue4_ddr_start; ptr < queue4_ddr_end; ptr += 0x100000)
	{
		Xil_SetTlbAttributes(reinterpret_cast<INTPTR>(ptr), DEVICE_MEMORY | EXECUTE_NEVER);
	}

	for(uint8_t* ptr = vdisk_ddr_start; ptr < vdisk_ddr_end; ptr += 0x100000)
	{
		Xil_SetTlbAttributes(reinterpret_cast<INTPTR>(ptr), NORM_NONCACHE | SHAREABLE | EXECUTE_NEVER);
	}

	dsb();
	isb();
}

bool MapMemory(uint32_t physAddr, uint32_t virtAddr, uint32_t sizeBytes)
{
	uint32_t numSections;
	uint32_t ttbr0;
	uint32_t ttbBase;
	volatile uint32_t *ttb;
	uint32_t srcIdx;
	uint32_t dstIdx;
	uint32_t entry;
	uint32_t i;

	physAddr = ALIGN(physAddr, 0x100000, ALIGN_LOW);
	virtAddr = ALIGN(virtAddr, 0x100000, ALIGN_LOW);
	sizeBytes = ALIGN(sizeBytes, 0x100000, ALIGN_HIGH);
	numSections = sizeBytes >> 20;

	asm volatile("mrc p15, 0, %0, c2, c0, 0" : "=r"(ttbr0));
	ttbBase = ttbr0 & (~0x3FFFU);
	ttb = (volatile uint32_t*)ttbBase;

	srcIdx = physAddr >> 20;
	dstIdx = virtAddr >> 20;

	for (i = 0; i < numSections; ++i)
	{
		entry = ttb[srcIdx + i];
		if (entry == 0)
		{
			return false;
		}
		ttb[dstIdx + i] = entry;
	}

	dsb();

	i = 0;
	asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r"(i) : "memory");

	dsb();
	isb();

	return true;
}
