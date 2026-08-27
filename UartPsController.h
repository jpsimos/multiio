/*
 * UartController.h
 *
 *  Created on: Mar 8, 2026
 *      Author: jacob a psimos
 */

#ifndef SRC_UARTPSCONTROLLER_H_
#define SRC_UARTPSCONTROLLER_H_

#include <atomic>
#include "xuartps.h"
#include "Middleware.h"

#define UART_RECEIVE_BUFFER_SIZE 1024
#define UART_INTRS (XUARTPS_IXR_RXFULL | XUARTPS_IXR_RXOVR | XUARTPS_IXR_TOUT)

class UartPsController: public Middleware
{
public:
	static UartPsController* CreateInstance(const unsigned short deviceId, const unsigned char intrId);
	static UartPsController* GetInstance(const unsigned short deviceId);

private:
	UartPsController(const unsigned short deviceId, const unsigned char intrId);
	virtual ~UartPsController();
	static void UartIsr(UartPsController* uartPtr, unsigned int event, unsigned int byteCount);
	virtual void Loop();
public:
	bool RxRingIsEmpty();
	bool RxRingIsFull();
	unsigned int RxRingAvailable();
	void RxRingPush(const unsigned char c);
	unsigned char RxRingPop();
	unsigned int RxRingHead();
	unsigned int RxRingTail();

private:
	const unsigned short m_deviceId;
	const unsigned char m_intrId;
	XUartPs_Config* m_uartConfig;
	XUartPs m_uart;
	alignas(32) unsigned char m_receiveBuffer[UART_RECEIVE_BUFFER_SIZE];
	std::atomic<int> m_receiveBufferHead;
	std::atomic<int> m_receiveBufferTail;
};

#endif /* SRC_UARTPSCONTROLLER_H_ */
