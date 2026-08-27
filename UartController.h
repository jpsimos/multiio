/*
 * UartController.h
 *
 *  Created on: Mar 8, 2026
 *      Author: jpsim
 */

#ifndef SRC_UARTCONTROLLER_H_
#define SRC_UARTCONTROLLER_H_

#include <atomic>
#include "xuartps.h"
#include "Middleware.h"

#define UART_RECEIVE_BUFFER_SIZE 4096
#define UART_INTRS (XUARTPS_IXR_RXFULL | XUARTPS_IXR_RXOVR | XUARTPS_IXR_TOUT)

class UartController: public Middleware
{
public:
	static UartController* CreateInstance(const unsigned short deviceId, const unsigned char intrId);
	static UartController* GetInstance(const unsigned short deviceId);

private:
	UartController(const unsigned short deviceId, const unsigned char intrId);
	virtual ~UartController();
	static void UartIsr(UartController* uartPtr, unsigned int event, unsigned int byteCount);
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

#endif /* SRC_UARTCONTROLLER_H_ */
