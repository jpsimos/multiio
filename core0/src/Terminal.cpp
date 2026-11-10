/*
 * Terminal.cpp
 *
 *  Created on: Jan 25, 2025
 *      Author: jacob a psimos
 */

#include "xparameters.h"
#include <iostream>
#include <string>
#include <sstream>
#include "xuartps.h"
#include "Helpers.h"
#include "Terminal.h"
#include "CpuMailbox.h"
#include "CpuMessage.h"
#include "InterruptManager.h"
#include "Platform.h"

using namespace Helpers;

Terminal* Terminal::CreateInstance(
	const unsigned short serialDeviceId,
	const unsigned char serialIntrId
)
{
	if(NULL == Terminal::sm_terminal)
	{
		Terminal::sm_terminal = new Terminal(serialDeviceId, serialIntrId);
	}
	return Terminal::sm_terminal;
}

Terminal* Terminal::GetInstance()
{
	if(NULL == Terminal::sm_terminal)
	{
		RUNTIME_ERROR("Must call CreateInstance first");
	}

	return Terminal::sm_terminal;
}


Terminal::Terminal(
	const unsigned short serialDeviceId,
	const unsigned char serialIntrId
) :
	Middleware(Middleware::AbstractType::TERMINAL),
	m_serialDeviceId(serialDeviceId),
	m_serialIntrId(serialIntrId),
	m_uartPsConfigPtr(XUartPs_LookupConfig(serialDeviceId)),
	m_uartPs(),
	m_numBytesIn(0U),
	m_readLineStream(new std::ostringstream()),
	m_lastCommand(),
	m_lastCommandResult(COMMAND_IGNORE)
{
	s32 result;

	result = XUartPs_CfgInitialize(&m_uartPs, m_uartPsConfigPtr, m_uartPsConfigPtr->BaseAddress);

	if(XST_SUCCESS != result)
	{
		RUNTIME_ERROR("XUartPs_CfgInitialize = %d", result);
	}

	XUartPs_SetInterruptMask(&m_uartPs, 0);
	XUartPs_DisableUart(&m_uartPs);
	XUartPs_SetHandler(&m_uartPs, reinterpret_cast<XUartPs_Handler>(Terminal::UartIsr), this);
	XUartPs_SetOptions(&m_uartPs, XUARTPS_OPTION_RESET_TX
		| XUARTPS_OPTION_RESET_RX | XUARTPS_OPTION_RESET_TMOUT);
	XUartPs_SetFifoThreshold(&m_uartPs, 1);
	InterruptManager::GetInstance()->Attach(m_serialIntrId, InterruptManager::IntrType::PS_INTR,
		reinterpret_cast<Xil_InterruptHandler>(XUartPs_InterruptHandler), &m_uartPs, true);
	XUartPs_SetInterruptMask(&m_uartPs, XUARTPS_IXR_RXOVR | XUARTPS_IXR_TXEMPTY);
	XUartPs_SetOperMode(&m_uartPs, XUARTPS_OPER_MODE_NORMAL);
	XUartPs_EnableUart(&m_uartPs);

	std::cout <<
			ANSI_LIGHT_GREEN "MultiIO Core " << XPAR_CPU_ID
			<< " <" __DATE__ " " __TIME__ ">" ANSI_RESET "\n";
}

Terminal::~Terminal()
{
	XUartPs_DisableUart(&m_uartPs);
	InterruptManager::GetInstance()->Detach(m_serialIntrId, InterruptManager::IntrType::PS_INTR);
}

void Terminal::UartIsr(Terminal* terminalPtr, u32 Event, u32 EventData)
{
	if(XUARTPS_EVENT_RECV_DATA == Event
		&& XUartPs_IsReceiveData(terminalPtr->m_uartPsConfigPtr->BaseAddress))
	{
		terminalPtr->m_numBytesIn += 1;
	}
}

std::string Terminal::ReadLineCyclic()
{
	const unsigned int snapshotNumBytesIn = m_numBytesIn;
	std::string returnLine;
	int c;

	m_numBytesIn = 0;

	for(unsigned int i = 0; i < snapshotNumBytesIn; i++)
	{
		if(!std::cin.good())
		{
			std::cin.clear();
		}

		c = std::cin.get();

		if(c != -1)
		{
			if(c != '\r' && c != '\n')
			{
				*m_readLineStream << static_cast<char>(c);
			}
			else
			{
				returnLine = m_readLineStream->str();
				m_readLineStream.reset(new std::ostringstream());
				break;
			}
		}
		else
		{
			break;
		}
	}

	return returnLine;
}

void Terminal::Loop()
{
	int commandResult = COMMAND_IGNORE;
	std::string command;

	if(!m_nestedLock)
	{
		command = ReadLineCyclic();

		if(!command.empty())
		{
			if(command == ANSI_ARROW_UP && (m_lastCommandResult & COMMAND_OK))
			{
				command = m_lastCommand;
				commandResult |= COMMAND_LAST;
			}

			commandResult |= Middleware::ProcessCommandRegisteredMiddlewares(command);

			if((commandResult & (COMMAND_LAST | COMMAND_OK)) == COMMAND_OK)
			{
				m_lastCommand = command;
			}

			if(commandResult & COMMAND_OK)
			{
//				if(!AppendCommandToLog(command))
//				{
//					DEBUG_ERROR("AppendFile error");
//				}
			}

			m_lastCommandResult = commandResult;

			if((~commandResult & COMMAND_ERROR) && (commandResult & COMMAND_OK))
			{
				std::cout << ANSI_LIGHT_GREEN "OK" ANSI_RESET "\n";
			}
			else if(commandResult & COMMAND_ERROR)
			{
				std::cerr << ANSI_LIGHT_RED "ERR" ANSI_RESET "\n";
			}
			else if(commandResult == COMMAND_IGNORE)
			{
				std::cerr << ANSI_LIGHT_RED "UNK" ANSI_RESET "\n";
			}
		}
	}
}

int Terminal::ProcessCommand(const std::string& command)
{
	int commandResult = COMMAND_IGNORE;

	return commandResult;
}

