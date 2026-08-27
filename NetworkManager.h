/*
 * NetworkManager.h
 *
 *  Created on: Jul 12, 2025
 *      Author: jacob a psimos
 */

#ifndef SRC_NETWORKMANAGER_H_
#define SRC_NETWORKMANAGER_H_

#include "Middleware.h"
#include "Helpers.h"
#include "xemacps.h"
#include "lwip/tcp.h"

using namespace Helpers;

class NetworkManager: public Middleware
{
private:
	inline static NetworkManager* sm_networkManagerSingleton = nullptr;

public:
	static NetworkManager* CreateInstance(
		const unsigned short emacDeviceId,
		const unsigned char emacIntrId
	);

	static NetworkManager* GetInstance();

private:
	NetworkManager(
		const unsigned short emacDeviceId,
		const unsigned char emacIntrId
	);
	virtual ~NetworkManager();
	static void EmacIsr(NetworkManager* nmPtr);
	inline void ProcessTimers();
	unsigned short MdioRead(unsigned short reg);
	void MdioWrite(unsigned short reg, unsigned short value);
	void MdioBitModify(unsigned short reg, unsigned short mask, unsigned short value);

public:
	virtual void Loop();
	virtual int ProcessCommand(const std::string& command);

private:
	const unsigned short m_emacDeviceId;
	const unsigned char m_emacIntrId;
	XEmacPs* m_emacPtr;
	XEmacPs_Config* m_emacConfig;
	XEmacPs m_emac;
	netif m_netInterface;
	Time::Stopwatch m_arpTimer;
	Time::Stopwatch m_tcpFastTimer;
	Time::Stopwatch m_tcpSlowTimer;
	Time::Stopwatch m_dhcpCoarseTimer;
	Time::Stopwatch m_dhcpFineTimer;
};

#endif /* SRC_NETWORKMANAGER_H_ */
