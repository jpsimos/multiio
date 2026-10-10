/*
 * UartController.h
 *
 *  Created on: Mar 8, 2026
 *      Author: jpsim
 */

#ifndef SRC_UARTCONTROLLER_H_
#define SRC_UARTCONTROLLER_H_

#include "xuartps.h"
#include "Middleware.h"

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
	bool Available();
	unsigned int GetIntrStatus();
	void CancelRead();

private:
	const unsigned short m_deviceId;
	const unsigned char m_intrId;
	XUartPs_Config* m_uartConfig;
	XUartPs m_uart;
	alignas(32) unsigned char m_receiveBuffer[4096];
	bool m_reading;
	bool m_doneReading;
	bool m_excludeFromMainLoop;
};

#endif /* SRC_UARTCONTROLLER_H_ */
