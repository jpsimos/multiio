/*
 * MultiIO
 *
 * Main.cpp
 *
 * Author Jacob A Psimos
 */

#include "xparameters.h"
#include "xil_mmu.h"
#include "Helpers.h"
#include "InterruptManager.h"
#include "CpuMailbox.h"
#include "CpuMessage.h"
#include "Terminal.h"
#include "HmiController.h"
#include "HwClock.h"
#include "PowerController.h"
#include "Platform.h"
#include <iostream>
#include <string>

using namespace Helpers;

void EnableCpu1();
void ConfigureOcm();

int main(void)
{
	ConfigureOcm();

	InterruptManager::CreateInstance(
		XPAR_AXI_INTC_0_DEVICE_ID,
		XPAR_FABRIC_INTC_0_VEC_ID
	);

	CpuMailbox::CreateInstance(
		XPAR_MAILBOX_0_IF_0_DEVICE_ID,
		XPAR_INTC_0_MBOX_0_INTERRUPT_0_VEC_ID
	)->Register();

	Terminal::CreateInstance(
		XPAR_PS7_UART_0_DEVICE_ID,
		XPS_UART0_INT_ID
	)->Register();

	HwClock::CreateInstance(
		XPAR_MULTIIO_REAL_TIME_CLOCK_AXI_IIC_0_DEVICE_ID,
		XPAR_AXI_INTC_0_MULTIIO_REAL_TIME_CLOCK_AXI_IIC_0_IIC2INTC_IRPT_INTR,
		XPAR_MULTIIO_REAL_TIME_CLOCK_AXI_GPIO_0_DEVICE_ID,
		XPAR_AXI_INTC_0_MULTIIO_REAL_TIME_CLOCK_AXI_GPIO_0_IP2INTC_IRPT_INTR,
		0x68U
	)->Register();

	HmiController::CreateInstance(
		XPAR_MULTIIO_HMI_AXI_GPIO_1_DEVICE_ID,
		XPAR_AXI_INTC_0_MULTIIO_HMI_AXI_GPIO_1_IP2INTC_IRPT_INTR,
		XPAR_MULTIIO_HMI_AXI_GPIO_0_DEVICE_ID
	)->Register();

/*	PowerController::CreateInstance(
		XPAR_MULTIIO_PWRCTRL_AXI_GPIO_0_DEVICE_ID,
		XPAR_AXI_INTC_0_MULTIIO_PWRCTRL_AXI_GPIO_0_IP2INTC_IRPT_INTR,
		XPAR_TMRCTR_0_DEVICE_ID,
		XPAR_AXI_INTC_0_AXI_TIMER_0_INTERRUPT_INTR
	)->Register();*/

	CpuMailbox::GetInstance()->RegisterMessageHandler(
		CpuMessage::MessageType::Cpu1Loaded,
		[](CpuMessage& cpuMsg, void* arg) -> bool{
			std::cout << "Cpu1 Loaded\n";
			return true;
		},
		NULL
	);

	EnableCpu1();

	do
	{
		Middleware::LoopRegisteredMiddlewares();
	}
	while(1);

	return XST_SUCCESS;
}

void EnableCpu1()
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

