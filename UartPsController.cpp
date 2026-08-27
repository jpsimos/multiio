/*
 * UartController.cpp
 *
 *  Created on: Mar 8, 2026
 *      Author: jacob a psimos
 */

#include "UartPsController.h"
#include <array>
#include <sstream>
#include <atomic>
#include "Helpers.h"
#include "Interrupts.h"

static std::array<UartPsController*, XPAR_XUARTPS_NUM_INSTANCES> sm_uartControllers = {nullptr};

UartPsController* UartPsController::CreateInstance(const unsigned short deviceId, const unsigned char intrId)
{
	UartPsController* retPtr = nullptr;

	for(UartPsController* uartPtr : sm_uartControllers)
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
		retPtr = new UartPsController(deviceId, intrId);
	}

	return retPtr;
}

UartPsController* UartPsController::GetInstance(const unsigned short deviceId)
{
	for(UartPsController* uartPtr : sm_uartControllers)
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

UartPsController::UartPsController(const unsigned short deviceId, const unsigned char intrId)
	: Middleware(Middleware::AbstractType::TERMINAL),
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
	XUartPs_SetHandler(&m_uart, reinterpret_cast<XUartPs_Handler>(UartPsController::UartIsr), this);
	XUartPs_SetInterruptMask(&m_uart, UART_INTRS);
	XUartPs_SetFifoThreshold(&m_uart, 1);

	Interrupts::GetInstance()->Attach(intrId,
		reinterpret_cast<Xil_InterruptHandler>(XUartPs_InterruptHandler),
			&m_uart, true, 0xA0, false);

	XUartPs_WriteReg(m_uartConfig->BaseAddress, XUARTPS_CR_OFFSET,
		XUartPs_ReadReg(m_uartConfig->BaseAddress, XUARTPS_CR_OFFSET) | XUARTPS_CR_RXRST);
	XUartPs_EnableUart(&m_uart);
}

UartPsController::~UartPsController()
{
	XUartPs_DisableUart(&m_uart);
	XUartPs_SetInterruptMask(&m_uart, 0);
	Interrupts::GetInstance()->Detach(m_intrId);
}

bool UartPsController::RxRingIsEmpty()
{
	return m_receiveBufferHead.load(std::memory_order_acquire) == m_receiveBufferTail.load(std::memory_order_acquire);
}

bool UartPsController::RxRingIsFull()
{
	const unsigned int tail = m_receiveBufferTail.load(std::memory_order_acquire);
	const unsigned int head = m_receiveBufferHead.load(std::memory_order_acquire);
	return static_cast<bool>(((tail + 1) & (UART_RECEIVE_BUFFER_SIZE - 1)) == head);
}

unsigned int UartPsController::RxRingAvailable()
{
	const unsigned int head = m_receiveBufferHead.load(std::memory_order_acquire);
	const unsigned int tail = m_receiveBufferTail.load(std::memory_order_acquire);
	return static_cast<unsigned int>((tail - head) & (UART_RECEIVE_BUFFER_SIZE - 1));
}

void UartPsController::RxRingPush(const unsigned char c)
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
		nop();
	}
}

unsigned char UartPsController::RxRingPop()
{
	const unsigned int head = m_receiveBufferHead.load(std::memory_order_relaxed);
	unsigned char c = '\0';
	if(!RxRingIsEmpty())
	{
		c = m_receiveBuffer[head];
		m_receiveBufferHead.store((head + 1) & (UART_RECEIVE_BUFFER_SIZE - 1), std::memory_order_release);
	}
	return c;
}

unsigned int UartPsController::RxRingHead()
{
	const unsigned int head = m_receiveBufferHead.load(std::memory_order_acquire);
	return head;
}

unsigned int UartPsController::RxRingTail()
{
	const unsigned int tail = m_receiveBufferTail.load(std::memory_order_acquire);
	return tail;
}

void UartPsController::UartIsr(UartPsController* uartPtr, unsigned int event, unsigned int byteCount)
{
	unsigned char c;

	if(uartPtr != nullptr)
	{
		if(event == XUARTPS_EVENT_RECV_DATA || event == XUARTPS_EVENT_RECV_TOUT)
		{
			while(XUartPs_IsReceiveData(uartPtr->m_uartConfig->BaseAddress))
			{
				c = XUartPs_RecvByte(uartPtr->m_uartConfig->BaseAddress);

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

void UartPsController::Loop()
{
	bool gotLine = false;
	std::string::size_type lineLength;
	std::string line;

	if(!RxRingIsEmpty())
	{
		lineLength = 0;

		for(unsigned int i = RxRingHead(); i != RxRingTail(); i = ((i + 1) & (UART_RECEIVE_BUFFER_SIZE - 1)))
		{
			if(m_receiveBuffer[i] == '\0')
			{
				break;
			}
			if(m_receiveBuffer[i] == '\n')
			{
				gotLine = true;
				break;
			}
			lineLength++;
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



