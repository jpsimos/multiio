/*
 * Terminal.h
 *
 *  Created on: Jan 25, 2025
 *      Author: jacob a psimos
 */

#ifndef SRC_TERMINAL_H_
#define SRC_TERMINAL_H_

#include "xuartps.h"
#include "Helpers.h"
#include "Middleware.h"
#include "CpuMessage.h"
#include <string>
#include <sstream>
#include <queue>

using namespace Helpers;

class Terminal: public Middleware
{
private:
	static inline Terminal* sm_terminal = NULL;

public:
	static Terminal* CreateInstance(
		const unsigned short serialDeviceId,
		const unsigned char serialIntrId
	);
	static Terminal* GetInstance();

private:
	Terminal(
		const unsigned short serialDeviceId,
		const unsigned char serialIntrId
	);
	virtual ~Terminal();

protected:
	virtual void Loop();
	virtual int ProcessCommand(const std::string& command);

private:
	static void UartIsr(Terminal* terminalPtr, u32 Event, u32 EventData);
	std::string ReadLineCyclic();

private:
	const unsigned short m_serialDeviceId;
	const unsigned char m_serialIntrId;
	XUartPs_Config* m_uartPsConfigPtr;
	XUartPs m_uartPs;
	volatile unsigned int m_numBytesIn;
	std::unique_ptr<std::ostringstream> m_readLineStream;
	std::string m_lastCommand;
	int m_lastCommandResult;
};

#endif /* SRC_TERMINAL_H_ */
