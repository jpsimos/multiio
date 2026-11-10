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
#include "SdCardController.h"
#include "SpiFlash.h"
#include "NetworkManager.h"
#include "CpuMailbox.h"
#include <iostream>
#include <string>

using namespace Helpers;

int main(void)
{
	std::cout << "CPU 1 Loaded\n";

	InterruptManager::CreateInstance(
		XPAR_AXI_INTC_1_DEVICE_ID,
		XPAR_FABRIC_INTC_1_VEC_ID
	);

	CpuMailbox::CreateInstance(
		XPAR_MAILBOX_0_IF_1_DEVICE_ID,
		XPAR_INTC_1_MBOX_0_INTERRUPT_1_VEC_ID
	)->Register();

	SdCardController::CreateInstance(
		XPAR_PS7_SD_0_DEVICE_ID
	)->Register();

	SpiFlash::CreateInstance(
		XPAR_PS7_QSPI_0_DEVICE_ID
	)->Register();

	NetworkManager::CreateInstance(
		XPAR_PS7_ETHERNET_0_DEVICE_ID
	)->Register();

	{
		CpuMessage cpu1LoadedMsg(CpuMessage::MessageType::Cpu1Loaded);
		CpuMailbox::GetInstance()->QueueMessage(cpu1LoadedMsg);
	}

	do
	{
		Middleware::LoopRegisteredMiddlewares();
	}
	while(1);

	return XST_SUCCESS;
}

