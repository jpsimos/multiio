/*
 * NetworkManager.cpp
 *
 *  Created on: Jul 12, 2025
 *      Author: jacob a psimos
 */

#include "xparameters.h"
#include "NetworkManager.h"
#include "InterruptManager.h"
#include "GpioController.h"
#include "lwip/init.h"
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/inet.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/timeouts.h"
#include "netif/xadapter.h"
#include "netif/xemacpsif.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ios>

using namespace Helpers;

static const unsigned char EthernetDefaultMacAddress[6] =  { 0xD8, 0xCB, 0x3A, 0x39, 0xD5, 0x24 };

NetworkManager* NetworkManager::CreateInstance(
	const unsigned short emacDeviceId,
	const unsigned char emacIntrId
)
{
	if(NetworkManager::sm_networkManagerSingleton == nullptr)
	{
		NetworkManager::sm_networkManagerSingleton = new NetworkManager(emacDeviceId, emacIntrId);
	}

	return NetworkManager::sm_networkManagerSingleton;
}

NetworkManager* NetworkManager::GetInstance()
{
	if(NetworkManager::sm_networkManagerSingleton == nullptr)
	{
		RUNTIME_ERROR("Must call CreateInstance first.");
	}

	return NetworkManager::sm_networkManagerSingleton;
}

NetworkManager::NetworkManager(
	const unsigned short emacDeviceId,
	const unsigned char emacIntrId
) :
	Middleware(Middleware::AbstractType::NETWORK_MANAGER),
	m_emacDeviceId(emacDeviceId),
	m_emacIntrId(emacIntrId),
	m_emacPtr(nullptr),
	m_emacConfig(XEmacPs_LookupConfig(emacDeviceId)),
	m_netInterface(),
	m_arpTimer(),
	m_tcpFastTimer(),
	m_tcpSlowTimer(),
	m_dhcpCoarseTimer(),
	m_dhcpFineTimer()
{
	err_t err = ERR_IF;

	if(nullptr == m_emacConfig)
	{
		RUNTIME_ERROR("null config");
	}

	GpioController::GetInstance()->SetPinMode(GEM0_PHY_RSTN_PIN, PS_GPIO_OUT, PS_GPIO_LOW);
	Yield(10000);
	GpioController::GetInstance()->SetPin(GEM0_PHY_RSTN_PIN);

	lwip_init();

	InterruptManager::GetInstance()->Attach(
		m_emacIntrId, reinterpret_cast<Xil_InterruptHandler>(NetworkManager::EmacIsr),
			this, false, 0xA0, false);

	if(nullptr == xemac_add(&m_netInterface, 0, 0, 0, const_cast<unsigned char*>(&EthernetDefaultMacAddress[0]), m_emacConfig->BaseAddress))
	{
		RUNTIME_ERROR("xemac_add");
	}

	xemac_s *xemac = (xemac_s*)m_netInterface.state;
	xemacpsif_s *xemacps = (xemacpsif_s*)xemac->state;
	m_emacPtr = &(xemacps->emacps);

	netif_set_default(&m_netInterface);
	netif_set_up(&m_netInterface);
	err = dhcp_start(&m_netInterface);

	if(ERR_OK != err)
	{
		DEBUG_ERROR("dhcp_start = %d", err);
	}

	m_tcpFastTimer.Reset();
	m_tcpSlowTimer.Reset();
	m_dhcpCoarseTimer.Reset();
	m_dhcpFineTimer.Reset();

	InterruptManager::GetInstance()->Enable(m_emacIntrId);
}

NetworkManager::~NetworkManager()
{
	dhcp_release_and_stop(&m_netInterface);
	netif_set_down(&m_netInterface);
	netif_remove(&m_netInterface);
}

void NetworkManager::EmacIsr(NetworkManager* nmPtr)
{
	if(nmPtr != nullptr)
	{
		if(nmPtr->m_emacPtr != nullptr)
		{
			XEmacPs_IntrHandler(nmPtr->m_emacPtr);
		}
	}
}

void NetworkManager::ProcessTimers()
{
	if(m_arpTimer >= 1000000)
	{
		etharp_tmr();
		m_arpTimer.Reset();
	}

	if(m_tcpFastTimer >= 250000)
	{
		tcp_fasttmr();
		m_tcpFastTimer.Reset();
	}

	if(m_tcpSlowTimer >= 500000)
	{
		tcp_fasttmr();
		m_tcpFastTimer.Reset();
	}

	if(m_dhcpCoarseTimer >= 60000000)
	{
		dhcp_coarse_tmr();
		m_dhcpCoarseTimer.Reset();
	}

	if(m_dhcpFineTimer >= 500000)
	{
		dhcp_fine_tmr();
		m_dhcpFineTimer.Reset();
	}
}

void NetworkManager::Loop()
{
	if(netif_is_link_up(&m_netInterface))
	{
		xemacif_input(&m_netInterface);
	}

	ProcessTimers();
}

int NetworkManager::ProcessCommand(const std::string& command)
{
	int commandResult = COMMAND_IGNORE;

	if(command.length() > 4 && Text::StringBeginsWith(command, "net "))
	{
		if(command == "net status")
		{
			char ip[IP4ADDR_STRLEN_MAX];
			unsigned char* m_macAddress = &m_netInterface.hwaddr[0];

			ip4addr_ntoa_r(&m_netInterface.ip_addr, &ip[0], sizeof(ip));

			std::cout << "MAC: " << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[0]
				<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[1]
				<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[2]
				<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[3]
				<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[4]
				<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[5]
				<< "\nIP: " << ip << "\n";

			std::cout << "Link: 0x" << std::hex << (unsigned int)MdioRead(30) << "\n";
			commandResult = COMMAND_OK;
		}
	}

	return commandResult;
}


unsigned short NetworkManager::MdioRead(unsigned short reg)
{
	unsigned short value = 0U;
	int status = XEmacPs_PhyRead(m_emacPtr, 1, static_cast<u32>(reg), &value);

	if(XST_SUCCESS != status)
	{
		RUNTIME_ERROR("PhyRead = %x", status);
	}

	return value;
}

void NetworkManager::MdioWrite(unsigned short reg, unsigned short value)
{
	int status;

	status = XEmacPs_PhyWrite(m_emacPtr, 1, static_cast<u32>(reg), value);

	if(XST_SUCCESS != status)
	{
		RUNTIME_ERROR("PhyWrite = %x", status);
	}
}

void NetworkManager::MdioBitModify(unsigned short reg, unsigned short mask, unsigned short value)
{
	uint16_t mvalue = MdioRead(reg) & ~mask;
	value &= mask;
	mvalue |= value;
	MdioWrite(reg, mvalue);
}

