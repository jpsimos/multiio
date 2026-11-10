/*
 * NetworkManager.cpp
 *
 *  Created on: Jul 12, 2025
 *      Author: jacob a psimos
 */

#include "xparameters.h"
#include "NetworkManager.h"
#include "SdCardController.h"
#include "lwip/init.h"
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/inet.h"
#include "lwip/dhcp.h"
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

static const unsigned char EthernetDefaultMacAddress[XEMACPS_MAC_ADDR_SIZE] =  { 0x00, 0xD8, 0x61, 0xAA, 0xDA, 0x85 };

NetworkManager* NetworkManager::CreateInstance(const unsigned short emacDeviceId)
{
	NetworkManager* networkManager = NULL;

	if(emacDeviceId < XPAR_XEMACPS_NUM_INSTANCES && NULL == NetworkManager::sm_instances[emacDeviceId])
	{
		networkManager = new NetworkManager(emacDeviceId);
		NetworkManager::sm_instances[emacDeviceId] = networkManager;
	}

	return networkManager;
}

NetworkManager* NetworkManager::GetInstance(const unsigned short emacDeviceId)
{
	NetworkManager* networkManager = NULL;

	if(emacDeviceId < XPAR_XEMACPS_NUM_INSTANCES)
	{
		networkManager = NetworkManager::sm_instances[emacDeviceId];

		if(NULL == networkManager)
		{
			DEBUG_ERROR("null instance");
		}
	}

	return networkManager;
}

NetworkManager::NetworkManager(const unsigned short emacDeviceId) :
	Middleware(Middleware::AbstractType::NETWORK_MANAGER),
	m_emacBaseAddress(0U),
	m_emacPtr(NULL),
	m_netInterface(),
	m_macAddress(),
	m_arpTimer(),
	m_tcpTimer(),
	m_dhcpCoarseTimer(),
	m_dhcpFineTimer(),
	m_dhcpTimer(),
	m_configureTimer(),
	m_httpTcpPcb(NULL),
	m_httpTcpReceiveStream(new std::stringstream()),
	m_configured(false),
	m_httpEnable(true),
	m_httpRequestComplete(false),
	m_verbose(true)
{
	err_t err;
	XEmacPs_Config* emacConfig;

	std::memcpy(m_macAddress, EthernetDefaultMacAddress, XEMACPS_MAC_ADDR_SIZE);

	lwip_init();

	emacConfig = XEmacPs_LookupConfig(emacDeviceId);

	if(NULL == emacConfig)
	{
		RUNTIME_ERROR("XEmacPs_LookupConfig");
	}

	m_emacBaseAddress = emacConfig->BaseAddress;

	if(NULL == xemac_add(&m_netInterface, NULL, NULL, NULL, m_macAddress, m_emacBaseAddress))
	{
		RUNTIME_ERROR("xemac_add");
	}

	m_emacPtr = (XEmacPs*)&((xemacpsif_s*)((xemac_s*)m_netInterface.state)->state)->emacps;

	if(NULL == m_emacPtr)
	{
		RUNTIME_ERROR("NULL instance");
	}

	netif_set_default(&m_netInterface);
	netif_set_up(&m_netInterface);

	err = dhcp_start(&m_netInterface);

	if(ERR_OK != err)
	{
		DEBUG_LOG("dhcp_start = %s", NetworkManager::GetLwipErrorString(err).c_str());
	}

	m_arpTimer.Reset();
	m_tcpTimer.Reset();
	m_dhcpCoarseTimer.Reset();
	m_dhcpFineTimer.Reset();
	m_dhcpTimer.Reset();
	m_configureTimer.Reset();
}

NetworkManager::~NetworkManager()
{
	dhcp_release_and_stop(&m_netInterface);

	if(NULL != m_httpTcpPcb)
	{
		TcpClose(m_httpTcpPcb);
	}

	netif_set_down(&m_netInterface);
}

void NetworkManager::Loop()
{
	err_t err;
	unsigned int lastLoopMillis;

	MeasureLoopPerformance(&lastLoopMillis);

	if(lastLoopMillis > 250000)
	{
		DEBUG_ERROR("Middleware response time is too slow.");
	}

	if(netif_is_link_up(&m_netInterface))
	{
		xemacif_input(&m_netInterface);
	}

	if(m_arpTimer >= 1000000)
	{
		etharp_tmr();
		m_arpTimer.Reset();
	}

	if(m_tcpTimer >= 250000)
	{
		tcp_tmr();
		m_tcpTimer.Reset();
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

	if(!m_configured && m_configureTimer >= 1000000)
	{
		m_configured = true;
	}

	if(0 == m_netInterface.ip_addr.addr)
	{
		if(m_dhcpTimer >= 10000000)
		{
			err = dhcp_start(&m_netInterface);

			if(ERR_OK != err)
			{
				DEBUG_LOG("dhcp_start = %s", NetworkManager::GetLwipErrorString(err).c_str());
			}

			m_dhcpTimer.Reset();
		}
	}

	if(netif_is_link_up(&m_netInterface) && IsDhcpAquired() && m_httpEnable && NULL == m_httpTcpPcb)
	{
		m_httpTcpPcb = TcpListen(80);
	}
	else if(!m_httpEnable && NULL != m_httpTcpPcb)
	{
		TcpClose(m_httpTcpPcb);
		m_httpTcpPcb = NULL;
	}

//	static Time::Stopwatch statusTimer;
//
//	if(statusTimer >= 3.0)
//	{
//		std::cout << "MAC: " << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[0]
//			<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[1]
//			<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[2]
//			<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[3]
//			<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[4]
//			<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[5]
//			<< "\nDHCP Aquired: " << (IsDhcpAquired() ? "yes" : "no") << "\n"
//			<< "IP Address: " << GetIpAddressString(&m_netInterface.ip_addr) << "\n"
//			<< "HTTP: " << (m_httpTcpPcb ? "enabled" : "disabled") << "\n";
//
//		statusTimer.Reset();
//	}
}


bool NetworkManager::ProcessHttpRequest(std::string& request, tcp_pcb* tpcb)
{
	bool hadError = false;
	SdCardController* sdControllerPtr = SdCardController::GetInstance();
	std::vector<std::pair<std::string, std::string>> parameters;
	std::vector<std::pair<std::string, std::string>> urlParameters;
	std::string::size_type httpResponseLength;
	size_t numFileBytesRead;
	size_t indexFileSize;
	std::ostringstream httpResponse;
	std::ostringstream apiResponse;
	err_t err;
	int commandsResponse;
	std::unique_ptr<unsigned char[]> indexContents;

	ParseRequestParams(request, parameters, urlParameters);

	httpResponse << "HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n";

	if(urlParameters.empty())
	{
		httpResponse <<
			"Content-Type: text/html\r\n"
			"\r\n";

		indexFileSize = sdControllerPtr->GetFileSize("/www/index.html");
		indexContents.reset(new unsigned char[indexFileSize]);
		numFileBytesRead = sdControllerPtr->ReadFile("/www/index.html", indexContents.get(), indexFileSize);

		if(indexFileSize > 0 && indexFileSize == numFileBytesRead)
		{
			httpResponse.write(reinterpret_cast<const char*>(indexContents.get()), indexFileSize);
			httpResponseLength = Text::StringStreamLength(httpResponse);
			err = tcp_write(tpcb, httpResponse.str().c_str(), httpResponseLength, TCP_WRITE_FLAG_COPY);

			if(ERR_OK != err)
			{
				DEBUG_ERROR("tcp_write err = \"%s\"", NetworkManager::GetLwipErrorString(err).c_str());
				hadError = true;
			}
		}
	}
	else
	{
		httpResponse <<
			"Content-Type: text/json\r\n"
			"\r\n";

		for(const std::pair<std::string, std::string>& urlParameter : urlParameters)
		{
			if(urlParameter.first == "command")
			{
				commandsResponse = Middleware::ProcessCommandRegisteredMiddlewares(urlParameter.second);

				httpResponse << "{\"success\":" << (COMMAND_OK & commandsResponse ? "true" : "false")
					<< ",\"time\":" << Time::Seconds()
					<< ",\"message\":" << std::quoted(apiResponse.str())
					<< "}";
			}
		}

		httpResponseLength = Text::StringStreamLength(httpResponse);

		if(m_verbose)
		{
			std::cout << "\n" ANSI_LIGHT_BLUE << httpResponse.str() << ANSI_RESET << "\n";
		}

		err = tcp_write(tpcb, httpResponse.str().c_str(), httpResponseLength, TCP_WRITE_FLAG_COPY);

		if(ERR_OK != err)
		{
			DEBUG_ERROR("tcp_write err = %s", NetworkManager::GetLwipErrorString(err).c_str());
			hadError = true;
		}
	}

	return !hadError;
}

void NetworkManager::ParseRequestParams(
	const std::string& request,
	std::vector<std::pair<std::string, std::string>>& requestParameters,
	std::vector<std::pair<std::string, std::string>>& urlParameters
)
{
	std::vector<std::string> requestLines = Text::StringSplit(request, "\r\n");

	for(const std::string& requestLine : requestLines)
	{
		bool duplicate = false;

		if(requestLine.length() > 5 && Text::StringBeginsWith(requestLine, "GET /"))
		{
			for(const std::pair<std::string, std::string>& parameter : requestParameters)
			{
				if(parameter.first == "GET")
				{
					duplicate = true;
					break;
				}
			}

			if(!duplicate)
			{
				std::string::size_type httpProtocolWatermarkPos = requestLine.rfind("HTTP/");

				if(std::string::npos != httpProtocolWatermarkPos && 5 < httpProtocolWatermarkPos)
				{
					std::string url = requestLine.substr(4, httpProtocolWatermarkPos - 4 - 1);
					std::string::size_type firstParamPos = url.find("?");

					if(std::string::npos != firstParamPos)
					{
						std::vector<std::string> urlParameters_
							= Text::StringSplit(url.substr(firstParamPos + 1), "&");

						url = url.substr(0, firstParamPos);

						for(const std::string& urlParamWithValue : urlParameters_)
						{
							std::string::size_type equalSignPos = urlParamWithValue.find("=");

							if(std::string::npos != equalSignPos)
							{
								std::string urlParamName = urlParamWithValue.substr(0, equalSignPos);
								std::string urlParamValueEncoded = urlParamWithValue.substr(equalSignPos + 1);
								std::ostringstream urlParamValueDecoded;

								for(size_t i = 0; i < urlParamValueEncoded.length(); )
								{
									if(urlParamValueEncoded[i] == '%' && urlParamValueEncoded.length() > (i + 2))
									{
										char valueEncoded[3];
										valueEncoded[0] = urlParamValueEncoded[++i];
										valueEncoded[1] = urlParamValueEncoded[++i];
										valueEncoded[2] = '\0'; ++i;
										urlParamValueDecoded << static_cast<char>(
											std::strtol(valueEncoded, NULL, 16));
									}
									else
									{
										urlParamValueDecoded << urlParamValueEncoded[i++];
									}
								}

								urlParameters.push_back(std::make_pair<std::string, std::string>(
									urlParamWithValue.substr(0, equalSignPos),
									urlParamValueDecoded.str()
								));
							}
						}
					}

					requestParameters.push_back({ "GET", url });
				}
			}
		}
		else if(requestLine.length() > 6 && Text::StringBeginsWith(requestLine, "Host: "))
		{
			for(const std::pair<std::string, std::string>& parameter : requestParameters)
			{
				if(parameter.first == "Host")
				{
					duplicate = true;
					break;
				}
			}

			if(!duplicate)
			{
				requestParameters.push_back(std::make_pair<std::string, std::string>("Host",
					requestLine.substr(6)));
			}
		}
		else if(requestLine.length() > 12 && Text::StringBeginsWith(requestLine, "User-Agent: "))
		{
			for(const std::pair<std::string, std::string>& parameter : requestParameters)
			{
				if(parameter.first == "User-Agent")
				{
					duplicate = true;
					break;
				}
			}

			if(!duplicate)
			{
				requestParameters.push_back(std::make_pair<std::string, std::string>("User-Agent",
					requestLine.substr(12)));
			}
		}
		else if(requestLine.length() > 8 && Text::StringBeginsWith(requestLine, "Accept: "))
		{
			for(const std::pair<std::string, std::string>& parameter : requestParameters)
			{
				if(parameter.first == "Accept")
				{
					duplicate = true;
					break;
				}
			}

			if(!duplicate)
			{
				requestParameters.push_back(std::make_pair<std::string, std::string>("Accept",
					requestLine.substr(8)));
			}
		}
		else if(requestLine.length() > 12 && Text::StringBeginsWith(requestLine, "Connection: "))
		{
			for(const std::pair<std::string, std::string>& parameter : requestParameters)
			{
				if(parameter.first == "Connection")
				{
					duplicate = true;
					break;
				}
			}

			if(!duplicate)
			{
				requestParameters.push_back(std::make_pair<std::string, std::string>("Connection",
					requestLine.substr(12)));
			}
		}
		else if(requestLine.length() > 10 && Text::StringBeginsWith(requestLine, "Priority: "))
		{
			for(const std::pair<std::string, std::string>& parameter : requestParameters)
			{
				if(parameter.first == "Priority")
				{
					duplicate = true;
					break;
				}
			}

			if(!duplicate)
			{
				requestParameters.push_back(std::make_pair<std::string, std::string>("Priority",
					requestLine.substr(10)));
			}
		}
	}

	if(m_verbose)
	{
		for(const std::pair<std::string, std::string>& parameter : requestParameters)
		{
			std::cout << "REQUEST[" << parameter.first << "] = " << parameter.second << "\n";
		}

		for(const std::pair<std::string, std::string>& urlParameter : urlParameters)
		{
			std::cout << "URL_PARAMETER[" << std::quoted(urlParameter.first) << "] = " << urlParameter.second << "\n";
			Text::PrintHexTable(urlParameter.second.c_str(), urlParameter.second.length());
		}
	}
}

bool NetworkManager::IsDhcpAquired()
{
	return static_cast<bool>(m_netInterface.ip_addr.addr != 0 && dhcp_supplied_address(&m_netInterface));
}

tcp_pcb* NetworkManager::TcpListen(unsigned short port)
{
	tcp_pcb* pcb;
	err_t err;

	pcb = tcp_new();

	if(!pcb)
	{
		RUNTIME_ERROR("tcp_new out of memory");
	}

	tcp_err(pcb, reinterpret_cast<tcp_err_fn>(NetworkManager::TcpError));

	err = tcp_bind(pcb, IP_ADDR_ANY, (u16_t)port);

	if(ERR_OK != err)
	{
		RUNTIME_ERROR("tcp_bind = %x", (unsigned int)err);
	}

	tcp_arg(pcb, this);

	pcb = tcp_listen(pcb);

	if(!pcb)
	{
		RUNTIME_ERROR("tcp_listen out of memory");
	}

	tcp_accept(pcb, reinterpret_cast<tcp_accept_fn>(NetworkManager::TcpAcceptProc));

	return pcb;
}

err_t NetworkManager::TcpError(NetworkManager* ethMgr, err_t err)
{
	if(ethMgr->m_verbose)
	{
		DEBUG_LOG("err=%s", NetworkManager::GetLwipErrorString(err).c_str());
	}
	return err;
}

err_t NetworkManager::TcpAcceptProc(NetworkManager* ethMgr, tcp_pcb* newpcb, err_t err)
{
	if(ethMgr->m_verbose)
	{
		DEBUG_LOG("err=%s, state=%s, src=%s:%u, dst=%s:%u",
			NetworkManager::GetLwipErrorString(err).c_str(),
			NetworkManager::GetTcpStateString(newpcb->state).c_str(),
			NetworkManager::GetIpAddressString(&(newpcb->local_ip)).c_str(),
			newpcb->local_port,
			NetworkManager::GetIpAddressString(&(newpcb->remote_ip)).c_str(),
			newpcb->remote_port
		);
	}

	tcp_arg(newpcb, ethMgr);
	tcp_recv(newpcb, reinterpret_cast<tcp_recv_fn>(NetworkManager::TcpReceivedProc));
	tcp_sent(newpcb, reinterpret_cast<tcp_sent_fn>(NetworkManager::TcpSentProc));
	tcp_poll(newpcb, reinterpret_cast<tcp_poll_fn>(NetworkManager::TcpPollProc), 2); // 2 x 500msec = 1sec
	tcp_err(newpcb, reinterpret_cast<tcp_err_fn>(NetworkManager::TcpError));
	return err;
}

err_t NetworkManager::TcpReceivedProc(NetworkManager* ethMgr, tcp_pcb* tpcb, pbuf* pkt, err_t err)
{
	unsigned char* payload = reinterpret_cast<unsigned char*>(pkt->payload);
	std::string http;

	if(ethMgr->m_verbose)
	{
		DEBUG_LOG("err=%s, state=%s, src=%s:%u, dst=%s:%u, len=%u",
			NetworkManager::GetLwipErrorString(err).c_str(),
			NetworkManager::GetTcpStateString(tpcb->state).c_str(),
			NetworkManager::GetIpAddressString(&(tpcb->local_ip)).c_str(),
			tpcb->local_port,
			NetworkManager::GetIpAddressString(&(tpcb->remote_ip)).c_str(),
			tpcb->remote_port,
			(NULL != pkt ? pkt->len : 0U)
		);
	}

	if(NULL != pkt)
	{
		tcp_recved(tpcb, pkt->len);

		if(ESTABLISHED == tpcb->state)
		{
			for(unsigned short i = 0; i < pkt->len; i++)
			{
				*(ethMgr->m_httpTcpReceiveStream) << static_cast<char>(payload[i]);
			}

			if(pkt->len > 5)
			{
				if(payload[pkt->len - 4] == '\r' && payload[pkt->len - 3] == '\n'
					&& payload[pkt->len - 2] == '\r' && payload[pkt->len - 1] == '\n')
				{
					http = ethMgr->m_httpTcpReceiveStream->str();
					ethMgr->m_httpTcpReceiveStream.reset(new std::stringstream());
					ethMgr->m_httpRequestComplete = true;
				}
				else
				{
					ethMgr->m_httpRequestComplete = false;
				}
			}

			if(ethMgr->m_httpRequestComplete && !http.empty())
			{
				ethMgr->ProcessHttpRequest(http, tpcb);
			}
		}
		else if(CLOSE_WAIT == tpcb->state)
		{
			ethMgr->TcpClose(tpcb);
		}
	}

	if(NULL == pkt)
	{
		ethMgr->TcpClose(tpcb);
	}
	else
	{
		pbuf_free(pkt);
	}

	return err;
}

err_t NetworkManager::TcpSentProc(NetworkManager* ethMgr, tcp_pcb* tpcb, u16_t len)
{
	err_t err = ERR_OK;

	if(ethMgr->m_verbose)
	{
		DEBUG_LOG("state=%s, src=%s:%u, dst=%s:%u, len=%u",
			NetworkManager::GetTcpStateString(tpcb->state).c_str(),
			NetworkManager::GetIpAddressString(&(tpcb->local_ip)).c_str(),
			tpcb->local_port,
			NetworkManager::GetIpAddressString(&(tpcb->remote_ip)).c_str(),
			tpcb->remote_port,
			len
		);
	}

	if(ESTABLISHED == tpcb->state)
	{
		if(len > 0)
		{
			ethMgr->TcpClose(tpcb);
		}
	}
	else
	{
		ethMgr->TcpClose(tpcb);
	}

	return err;
}

err_t NetworkManager::TcpPollProc(NetworkManager* ethMgr, tcp_pcb *tpcb)
{
	err_t err = ERR_OK;


	if(ethMgr->m_verbose)
	{
		DEBUG_LOG("state=%s, acked=%u, src=%s:%u, dst=%s:%u",
			NetworkManager::GetTcpStateString(tpcb->state).c_str(),
			tpcb->bytes_acked,
			NetworkManager::GetIpAddressString(&(tpcb->local_ip)).c_str(),
			tpcb->local_port,
			NetworkManager::GetIpAddressString(&(tpcb->remote_ip)).c_str(),
			tpcb->remote_port
		);
	}

	return err;
}

void NetworkManager::TcpClose(tcp_pcb* pcb)
{
	err_t err;

	if(m_verbose)
	{
		DEBUG_LOG("closing src=%s:%u, dst=%s:%u",
			NetworkManager::GetIpAddressString(&(pcb->local_ip)).c_str(),
			pcb->local_port,
			NetworkManager::GetIpAddressString(&(pcb->remote_ip)).c_str(),
			pcb->remote_port
		);
	}

	tcp_accept(pcb, NULL);
	tcp_recv(pcb, NULL);
	tcp_sent(pcb, NULL);
	tcp_poll(pcb, NULL, 0);
	tcp_err(pcb, NULL);

	err = tcp_close(pcb);

	if(ERR_OK != err)
	{
		DEBUG_LOG("tcp_close = %s", NetworkManager::GetLwipErrorString(err).c_str());
	}
}

int NetworkManager::ProcessCommand(const std::string& command)
{
	int commandResult = COMMAND_IGNORE;
	unsigned int reg;
	unsigned int mask;
	unsigned int val;
	int numChars;

	if(Text::StringBeginsWith(command, "net "))
	{
		if(command == "net status")
		{
			std::cout << "MAC: " << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[0]
				<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[1]
				<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[2]
				<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[3]
				<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[4]
				<< ':' << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)m_macAddress[5]
				<< "\nDHCP Aquired: " << (IsDhcpAquired() ? "yes" : "no") << "\n"
				<< "IP Address: " << GetIpAddressString(&m_netInterface.ip_addr) << "\n"
				<< "HTTP: " << (m_httpTcpPcb ? "enabled" : "disabled") << "\n";
			commandResult = COMMAND_OK;
		}
		else if(3 == std::sscanf(command.c_str(), "net mdio modify %x %x %x", &reg, &mask, &val))
		{
			MdioBitModify(reg, mask, val);
			val = MdioRead(reg);
			std::cout << Text::Stringf("mdio[%x] = %x\n", reg, val);
			commandResult = COMMAND_OK;
		}
		else if(1 == std::sscanf(command.c_str(), "net mdio read %x", &reg))
		{
			val = MdioRead(reg);
			std::cout << Text::Stringf("mdio[%x] = %x\n", reg, val);
			commandResult = COMMAND_OK;
		}
		else if(2 == std::sscanf(command.c_str(), "net mdio write %x %x", &reg, &val))
		{
			MdioWrite(reg, val);
			std::cout << Text::Stringf("mdio[%x] = %x\n", reg, val);
			commandResult = COMMAND_OK;
		}
		else if(command == "net enable http")
		{
			m_httpEnable = true;
			commandResult = COMMAND_OK;
		}
		else if(command == "net disable http")
		{
			m_httpEnable = false;

			if(NULL != m_httpTcpPcb)
			{
				TcpClose(m_httpTcpPcb);
				m_httpTcpPcb = NULL;
			}

			commandResult = COMMAND_OK;
		}
		else if(command == "net enable verbose")
		{
			m_verbose = true;
			commandResult = COMMAND_OK;
		}
		else if(command == "net disable verbose")
		{
			m_verbose = false;
			commandResult = COMMAND_OK;
		}
	}

	return commandResult;
}

std::string NetworkManager::GetLwipErrorString(const err_t err)
{
	static const char* const lwipErrors[] = {
		"ERR_OK",
		"ERR_MEM",
		"ERR_BUF",
		"ERR_TIMEOUT",
		"ERR_RTE",
		"ERR_INPROGRESS",
		"ERR_VAL",
		"ERR_WOULDBLOCK",
		"ERR_USE",
		"ERR_ALREADY",
		"ERR_ISCONN",
		"ERR_CONN",
		"ERR_IF",
		"ERR_ABRT",
		"ERR_RST",
		"ERR_CLSD",
		"ERR_ARG",
		"ERR_UNKNOWN"
	};

	const int index = std::min((int)(sizeof(lwipErrors) / sizeof(const char*)) - 1, std::abs((int)err));

	return lwipErrors[index];
}

std::string NetworkManager::GetTcpStateString(const tcp_state state)
{
	return tcp_debug_state_str(state);
}

std::string NetworkManager::GetIpAddressString(const ip_addr_t* const ip)
{
	unsigned int ipaddr = (unsigned int)ip->addr;
	std::ostringstream os;

	os << (ipaddr & 0xFF) << '.' << ((ipaddr >> 8) & 0xFF) << '.'
			<< ((ipaddr >> 16) & 0xFF) << '.' << ((ipaddr >> 24) & 0xFF);

	return os.str();
}

unsigned short NetworkManager::MdioRead(unsigned short reg)
{
	unsigned short value = 0U;
	int status = XEmacPs_PhyRead(m_emacPtr, 1, static_cast<u32>(reg), &value);

	if(XST_SUCCESS != status)
	{
		RUNTIME_ERROR("XEmacPs_PhyRead = %x", status);
	}

	return value;
}

void NetworkManager::MdioWrite(unsigned short reg, unsigned short value)
{
	int status;

	status = XEmacPs_PhyWrite(m_emacPtr, 1, static_cast<u32>(reg), value);

	if(XST_SUCCESS != status)
	{
		RUNTIME_ERROR("XEmacPs_PhyWrite = %x", status);
	}
}

void NetworkManager::MdioBitModify(unsigned short reg, unsigned short mask, unsigned short value)
{
	uint16_t mvalue = MdioRead(reg);
	mvalue &= ~mask;
	mvalue |= value & mask;
	MdioWrite(reg, mvalue);
}
