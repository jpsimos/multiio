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
#include "netif/xadapter.h"
#include "lwip/tcp.h"
#include <memory>

using namespace Helpers;

class NetworkManager: public Middleware
{
private:
	inline static NetworkManager* sm_instances[XPAR_XEMACPS_NUM_INSTANCES] = {};

public:
	static NetworkManager* CreateInstance(const unsigned short emacDeviceId);
	static NetworkManager* GetInstance(const unsigned short emacDeviceId);

private:
	NetworkManager(const unsigned short emacDeviceId);
	virtual ~NetworkManager();
	static err_t TcpError(NetworkManager* ethMgr, err_t err);
	static err_t TcpAcceptProc(NetworkManager* ethMgr, tcp_pcb* newpcb, err_t err);
	static err_t TcpReceivedProc(NetworkManager* ethMgr, tcp_pcb* tpcb, pbuf* pkt, err_t err);
	static err_t TcpSentProc(NetworkManager* ethMgr, tcp_pcb* tpcb, u16_t len);
	static err_t TcpPollProc(NetworkManager* ethMgr, tcp_pcb *tpcb);
	static void TimerIsr(NetworkManager* ethMgr, u8 TmrCtrNumber);
	bool ProcessHttpRequest(std::string& request, tcp_pcb* tpcb);
	void ParseRequestParams(
		const std::string& request,
		std::vector<std::pair<std::string, std::string>>& requestParameters,
		std::vector<std::pair<std::string, std::string>>& urlParameters
	);
	unsigned short MdioRead(unsigned short reg);
	void MdioWrite(unsigned short reg, unsigned short value);
	void MdioBitModify(unsigned short reg, unsigned short mask, unsigned short value);
	static std::string GetLwipErrorString(const err_t err);
	static std::string GetTcpStateString(const tcp_state state);
	static std::string GetIpAddressString(const ip_addr_t* const ip);

public:
	virtual void Loop();
	virtual int ProcessCommand(const std::string& command);
	bool IsDhcpAquired();
	tcp_pcb* TcpListen(unsigned short port);
	void TcpClose(tcp_pcb* pcb);

private:
	uintptr_t m_emacBaseAddress;
	XEmacPs* m_emacPtr;
	netif m_netInterface;
	unsigned char m_macAddress[XEMACPS_MAC_ADDR_SIZE];
	Time::Stopwatch m_arpTimer;
	Time::Stopwatch m_tcpTimer;
	Time::Stopwatch m_dhcpCoarseTimer;
	Time::Stopwatch m_dhcpFineTimer;
	Time::Stopwatch m_dhcpTimer;
	Time::Stopwatch m_configureTimer;
	tcp_pcb* m_httpTcpPcb;
	std::unique_ptr<std::stringstream> m_httpTcpReceiveStream;
	bool m_configured;
	bool m_httpEnable;
	bool m_httpRequestComplete;
	bool m_verbose;

};

#endif /* SRC_NETWORKMANAGER_H_ */
