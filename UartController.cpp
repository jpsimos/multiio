/*
 * UartController.cpp
 *
 *  Created on: Mar 8, 2026
 *      Author: jpsim
 */

#include <array>
#include <sstream>
#include "UartController.h"
#include "InterruptManager.h"

#define UART_INTRS (XUARTPS_IXR_RXOVR | XUARTPS_IXR_RXFULL | XUARTPS_IXR_OVER | XUARTPS_IXR_TOUT)

static std::array<UartController*, XPAR_XUARTPS_NUM_INSTANCES> sm_uartControllers = {nullptr};

UartController* UartController::CreateInstance(const unsigned short deviceId, const unsigned char intrId)
{
	UartController* retPtr = nullptr;

	for(UartController* uartPtr : sm_uartControllers)
	{
		if(uartPtr != nullptr)
		{
			if(deviceId == uartPtr->m_deviceId)
			{
				retPtr = uartPtr;
				RUNTIME_ERROR("Already CreatedInstance.");
			}
		}
	}

	if(retPtr == nullptr)
	{
		retPtr = new UartController(deviceId, intrId);
	}

	return retPtr;
}

UartController* UartController::GetInstance(const unsigned short deviceId)
{
	for(UartController* uartPtr : sm_uartControllers)
	{
		if(uartPtr != nullptr)
		{
			if(deviceId == uartPtr->m_deviceId)
			{
				return uartPtr;
			}
		}
	}

	RUNTIME_ERROR("Must call CreateInstance first.");

	return nullptr;
}

UartController::UartController(const unsigned short deviceId, const unsigned char intrId)
	: Middleware(Middleware::AbstractType::TERMINAL, MW_PRIORITY_NORMAL),
	  m_deviceId(deviceId),
	  m_intrId(intrId),
	  m_uartConfig(XUartPs_LookupConfig(deviceId)),
	  m_uart(),
	  m_receiveBuffer(),
	  m_reading(false),
	  m_doneReading(true),
	  m_excludeFromMainLoop(false)
{
	int result;

	result = XUartPs_CfgInitialize(&m_uart, m_uartConfig, m_uartConfig->BaseAddress);

	if(XST_SUCCESS != result)
	{
		RUNTIME_ERROR("XUartPs_CfgInitialize = %d", result);
	}

	XUartPs_SetOptions(&m_uart, XUARTPS_OPTION_RESET_TX
		| XUARTPS_OPTION_RESET_RX | XUARTPS_OPTION_RESET_TMOUT);

	XUartPs_SetFifoThreshold(&m_uart, 1);

	XUartPs_SetHandler(&m_uart, reinterpret_cast<XUartPs_Handler>(UartController::UartIsr), this);

	InterruptManager::GetInstance()->Attach(intrId,
		reinterpret_cast<Xil_InterruptHandler>(XUartPs_InterruptHandler),
			&m_uart, true, 0xA0, false);

	XUartPs_SetInterruptMask(&m_uart, UART_INTRS);
}

UartController::~UartController()
{
	XUartPs_DisableUart(&m_uart);
	XUartPs_SetInterruptMask(&m_uart, 0);
	InterruptManager::GetInstance()->Detach(m_intrId);
}

void UartController::UartIsr(UartController* uartPtr, unsigned int event, unsigned int byteCount)
{
	unsigned int intrStatus;

	if(uartPtr != nullptr)
	{
		intrStatus = uartPtr->GetIntrStatus();

		if(XUARTPS_EVENT_RECV_DATA == event)
		{
			if(byteCount == uartPtr->m_uart.ReceiveBuffer.RequestedBytes && uartPtr->m_reading)
			{
				uartPtr->m_doneReading = true;
			}
		}
	}
}

void UartController::Loop()
{
	bool gotLine;
	char* bufferPtr;

	if(!m_excludeFromMainLoop)
	{
		gotLine = false;

		if(!m_reading && m_doneReading)
		{
			m_reading = true;
			m_doneReading = false;

			if(sizeof(m_receiveBuffer) == XUartPs_Recv(&m_uart, m_receiveBuffer, sizeof(m_receiveBuffer)))
			{
				m_reading = false;
				m_doneReading = true;
			}
		}
		else if(m_reading && m_doneReading)
		{
			m_reading = false;
		}
		else if(m_reading && !m_doneReading)
		{
			nop();
		}

		for(bufferPtr = reinterpret_cast<char*>(&m_receiveBuffer[0]);
			bufferPtr < reinterpret_cast<char*>(m_uart.ReceiveBuffer.NextBytePtr); bufferPtr++)
		{
			if(*bufferPtr == '\r')
			{
				gotLine = true;
				break;
			}
		}

		if(gotLine)
		{
			m_excludeFromMainLoop = true;
			std::string line(reinterpret_cast<char*>(&m_receiveBuffer[0]), bufferPtr);
			CancelRead();
			Middleware::ProcessCommandRegisteredMiddlewares(line);
			m_excludeFromMainLoop = false;
		}
	}
}

bool UartController::Available()
{
	return static_cast<bool>(XUartPs_IsReceiveData(m_uartConfig->BaseAddress));
}

unsigned int UartController::GetIntrStatus()
{
	unsigned int IsrStatus;

	IsrStatus = XUartPs_ReadReg(m_uartConfig->BaseAddress,
			XUARTPS_IMR_OFFSET);

	IsrStatus &= XUartPs_ReadReg(m_uartConfig->BaseAddress,
			XUARTPS_ISR_OFFSET);

	return IsrStatus;
}

void UartController::CancelRead()
{
	XUartPs_DisableUart(&m_uart);

	XUartPs_SetInterruptMask(&m_uart, 0);

	XUartPs_SetOptions(&m_uart, XUARTPS_OPTION_RESET_RX);

	InterruptManager::GetInstance()->Disable(m_intrId);

	m_uart.ReceiveBuffer.RemainingBytes = 0;
	m_uart.ReceiveBuffer.RequestedBytes = 0;
	m_uart.ReceiveBuffer.NextBytePtr = nullptr;
	m_reading = false;
	m_doneReading = true;

	InterruptManager::GetInstance()->Enable(m_intrId);

	XUartPs_SetInterruptMask(&m_uart, UART_INTRS);

	XUartPs_EnableUart(&m_uart);
}
