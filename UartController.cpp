/*
 * UartController.cpp
 *
 *  Created on: Mar 8, 2026
 *      Author: jpsim
 */

#include <array>
#include <sstream>
#include <atomic>
#include "UartController.h"
#include "InterruptManager.h"
#include "Helpers.h"

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

	return nullptr;
}

UartController::UartController(const unsigned short deviceId, const unsigned char intrId)
	: Middleware(Middleware::AbstractType::TERMINAL, MW_PRIORITY_NORMAL),
	  m_deviceId(deviceId),
	  m_intrId(intrId),
	  m_uartConfig(XUartPs_LookupConfig(deviceId)),
	  m_uart(),
	  m_receiveBuffer(),
	  m_receiveBufferHead(),
	  m_receiveBufferTail()
{
	int result;

	static_assert(ISPOWEROF2(UART_RECEIVE_BUFFER_SIZE), "UART receive buffer is not sized properly.");


	std::memset(m_receiveBuffer, '\0', sizeof(m_receiveBuffer));

	result = XUartPs_CfgInitialize(&m_uart, m_uartConfig, m_uartConfig->BaseAddress);

	if(XST_SUCCESS != result)
	{
		RUNTIME_ERROR("CfgInitialize = %d", result);
	}

	XUartPs_SetOptions(&m_uart, XUARTPS_OPTION_RESET_TX
		| XUARTPS_OPTION_RESET_RX | XUARTPS_OPTION_RESET_TMOUT);
	XUartPs_SetHandler(&m_uart, reinterpret_cast<XUartPs_Handler>(UartController::UartIsr), this);
	XUartPs_SetInterruptMask(&m_uart, UART_INTRS);
	XUartPs_SetFifoThreshold(&m_uart, 1);

	InterruptManager::GetInstance()->Attach(intrId,
		reinterpret_cast<Xil_InterruptHandler>(XUartPs_InterruptHandler),
			&m_uart, true, 0xA0, false);

	XUartPs_WriteReg(m_uartConfig->BaseAddress, XUARTPS_CR_OFFSET,
		XUartPs_ReadReg(m_uartConfig->BaseAddress, XUARTPS_CR_OFFSET) | XUARTPS_CR_RXRST);
	XUartPs_EnableUart(&m_uart);
}

UartController::~UartController()
{
	XUartPs_DisableUart(&m_uart);
	XUartPs_SetInterruptMask(&m_uart, 0);
	InterruptManager::GetInstance()->Detach(m_intrId);
}

bool UartController::RxRingIsEmpty()
{
	return m_receiveBufferHead.load(std::memory_order_acquire) == m_receiveBufferTail.load(std::memory_order_acquire);
}

bool UartController::RxRingIsFull()
{
	const unsigned int tail = m_receiveBufferTail.load(std::memory_order_acquire);
	const unsigned int head = m_receiveBufferHead.load(std::memory_order_acquire);
	return static_cast<bool>(((tail + 1) & (UART_RECEIVE_BUFFER_SIZE - 1)) == head);
}

unsigned int UartController::RxRingAvailable()
{
	const unsigned int head = m_receiveBufferHead.load(std::memory_order_acquire);
	const unsigned int tail = m_receiveBufferTail.load(std::memory_order_acquire);
	return static_cast<unsigned int>((tail - head) & (UART_RECEIVE_BUFFER_SIZE - 1));
}

void UartController::RxRingPush(const unsigned char c)
{
	const unsigned int tail = m_receiveBufferTail.load(std::memory_order_relaxed);
	const unsigned int head = m_receiveBufferHead.load(std::memory_order_acquire);

	if(((tail + 1) & (UART_RECEIVE_BUFFER_SIZE - 1)) != head)
	{
		m_receiveBuffer[tail] = c;
		m_receiveBufferTail.store((tail + 1) & (UART_RECEIVE_BUFFER_SIZE - 1), std::memory_order_release);
	}
	else
	{
		// shouldn't happen
		DEBUG_LOG("Corner case detected");
	}

}

unsigned char UartController::RxRingPop()
{
	const unsigned int head = m_receiveBufferHead.load(std::memory_order_relaxed);
	const unsigned char c = m_receiveBuffer[head];
	m_receiveBufferHead.store((head + 1) & (UART_RECEIVE_BUFFER_SIZE - 1), std::memory_order_release);
	return c;
}

unsigned int UartController::RxRingHead()
{
	const unsigned int head = m_receiveBufferHead.load(std::memory_order_acquire);
	return head;
}

unsigned int UartController::RxRingTail()
{
	const unsigned int tail = m_receiveBufferTail.load(std::memory_order_acquire);
	return tail;
}

void UartController::UartIsr(UartController* uartPtr, unsigned int event, unsigned int byteCount)
{
	if(uartPtr != nullptr)
	{
		if(event == XUARTPS_EVENT_RECV_DATA || event == XUARTPS_EVENT_RECV_TOUT)
		{
			while(XUartPs_IsReceiveData(uartPtr->m_uartConfig->BaseAddress))
			{
				unsigned char c = XUartPs_RecvByte(uartPtr->m_uartConfig->BaseAddress);

				if(!XUartPs_IsTransmitFull(uartPtr->m_uartConfig->BaseAddress))
				{
					XUartPs_SendByte(uartPtr->m_uartConfig->BaseAddress, c);
				}

				if(c == '\r')
				{
					c = '\n';

					if(!XUartPs_IsTransmitFull(uartPtr->m_uartConfig->BaseAddress))
					{
						XUartPs_SendByte(uartPtr->m_uartConfig->BaseAddress, c);
					}
				}

				uartPtr->RxRingPush(c);
			}
		}
		else if(event == XUARTPS_EVENT_RECV_ERROR)
		{
			//XUartPs_WriteReg(uartPtr->m_uartConfig->BaseAddress, XUARTPS_CR_OFFSET, XUARTPS_CR_RXRST);
		}
	}
}

void UartController::Loop()
{
	bool gotLine = false;
	std::string::size_type lineLength;
	std::string line;

	if(!RxRingIsEmpty())
	{
		lineLength = 0;

		for(unsigned int i = RxRingHead(); i != RxRingTail(); i = ((i + 1) & (UART_RECEIVE_BUFFER_SIZE - 1)))
		{
			lineLength++;

			if(m_receiveBuffer[i] == '\n')
			{
				gotLine = true;
				break;
			}
		}

		if(gotLine)
		{
			line.reserve(lineLength);

			for(char c = static_cast<char>(RxRingPop());
				!RxRingIsEmpty() && c != '\n'; c = static_cast<char>(RxRingPop()))
			{
				line += c;
			}

			if(Middleware::ProcessCommandRegisteredMiddlewares(line) == COMMAND_IGNORE)
			{
				std::cout << ANSI_LIGHT_RED "Unknown Command: " ANSI_RESET << line << "\n";
			}
		}
	}
}



