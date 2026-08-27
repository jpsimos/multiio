/*
 * XAdc.cpp
 *
 *  Created on: Feb 22, 2025
 *      Author: jacob
 */

#include <iostream>
#include <string>
#include <limits>
#include "xparameters.h"
#include "XAdc.h"
#include "InterruptManager.h"
#include "Helpers.h"

using namespace Helpers;

XAdc* XAdc::CreateInstance(const unsigned short deviceId, const unsigned char intrId)
{
	if(nullptr == XAdc::sm_singletonPtr)
	{
		XAdc::sm_singletonPtr = new XAdc(deviceId, intrId);
	}
	return XAdc::sm_singletonPtr;
}

XAdc* XAdc::GetInstance()
{
	if(nullptr == XAdc::sm_singletonPtr)
	{
		RUNTIME_ERROR("Must call CreateInstance first");
	}
	return XAdc::sm_singletonPtr;
}

XAdc::XAdc(const unsigned short deviceId, const unsigned char intrId) :
	Middleware(Middleware::AbstractType::XADC),
	m_intrId(intrId),
	m_adcConfig(XAdcPs_LookupConfig(deviceId)),
	m_adc()
{
	int result;

	result = XAdcPs_CfgInitialize(&m_adc, m_adcConfig, m_adcConfig->BaseAddress);

	if(XST_SUCCESS != result)
	{
		RUNTIME_ERROR("CfgInitialize = %d", result);
	}

	InterruptManager::GetInstance()->Attach(m_intrId,
		reinterpret_cast<Xil_InterruptHandler>(XAdc::HandleIsr), this, true, 0xA0, false);

	XAdcPs_IntrClear(&m_adc, XADCPS_INTX_OT_MASK | XADCPS_INTX_ALM_ALL_MASK);
	XAdcPs_IntrEnable(&m_adc, XADCPS_INTX_OT_MASK | XADCPS_INTX_ALM_ALL_MASK);
}

XAdc::~XAdc()
{
	XAdcPs_IntrDisable(&m_adc, XADCPS_INTX_ALL_MASK);
	InterruptManager::GetInstance()->Detach(m_intrId);
}

void XAdc::HandleIsr(XAdc* adcPtr)
{
	nop();
}

double XAdc::ReadTemperatureF()
{
	double temperature = static_cast<double>(XAdcPs_RawToTemperature(
		XAdcPs_GetAdcData(&m_adc, XADCPS_CH_TEMP)));
	temperature *= 9.0;
	temperature /= 5.0;
	temperature += 32.0;
	return temperature;
}

double XAdc::ReadMinTemperatureF()
{
	double minTempF = static_cast<double>(XAdcPs_RawToTemperature(
		XAdcPs_GetMinMaxMeasurement(&m_adc, XADCPS_MIN_TEMP)));
	minTempF *= 9.0;
	minTempF /= 5.0;
	minTempF += 32.0;
	return minTempF;
}

double XAdc::ReadMaxTemperatureF()
{
	double maxTempF = static_cast<double>(XAdcPs_RawToTemperature(
		XAdcPs_GetMinMaxMeasurement(&m_adc, XADCPS_MAX_TEMP)));
	maxTempF *= 9.0;
	maxTempF /= 5.0;
	maxTempF += 32.0;
	return maxTempF;
}

void XAdc::ReadVoltages(double* outVccInt, double* outVccAux, double* outVBRAM)
{
	if(nullptr != outVccInt)
	{
		*outVccInt = static_cast<double>(XAdcPs_RawToVoltage(
			XAdcPs_GetAdcData(&m_adc, XADCPS_CH_VCCINT)));
	}

	if(nullptr != outVccAux)
	{
		*outVccAux = static_cast<double>(XAdcPs_RawToVoltage(
				XAdcPs_GetAdcData(&m_adc, XADCPS_CH_VCCAUX)));
	}

	if(nullptr != outVBRAM)
	{
		*outVBRAM = static_cast<double>(XAdcPs_RawToVoltage(
				XAdcPs_GetAdcData(&m_adc, XADCPS_CH_VBRAM)));
	}
}

void XAdc::ReadMinVoltages(double* outVccInt, double* outVccAux, double* outVBRAM)
{
	if(nullptr != outVccInt)
	{
		*outVccInt = static_cast<double>(XAdcPs_RawToVoltage(
			XAdcPs_GetMinMaxMeasurement(&m_adc, XADCPS_MIN_VCCINT)));
	}

	if(nullptr != outVccAux)
	{
		*outVccAux = static_cast<double>(XAdcPs_RawToVoltage(
			XAdcPs_GetMinMaxMeasurement(&m_adc, XADCPS_MIN_VCCAUX)));
	}

	if(nullptr != outVBRAM)
	{
		*outVBRAM = static_cast<double>(XAdcPs_RawToVoltage(
			XAdcPs_GetMinMaxMeasurement(&m_adc, XADCPS_MIN_VBRAM)));
	}
}

void XAdc::ReadMaxVoltages(double* outVccInt, double* outVccAux, double* outVBRAM)
{
	if(nullptr != outVccInt)
	{
		*outVccInt = static_cast<double>(XAdcPs_RawToVoltage(
			XAdcPs_GetMinMaxMeasurement(&m_adc, XADCPS_MAX_VCCINT)));
	}

	if(nullptr != outVccAux)
	{
		*outVccAux = static_cast<double>(XAdcPs_RawToVoltage(
			XAdcPs_GetMinMaxMeasurement(&m_adc, XADCPS_MAX_VCCAUX)));
	}

	if(nullptr != outVBRAM)
	{
		*outVBRAM = static_cast<double>(XAdcPs_RawToVoltage(
			XAdcPs_GetMinMaxMeasurement(&m_adc, XADCPS_MAX_VBRAM)));
	}
}

int XAdc::ProcessCommand(const std::string& command)
{
	int commandResult = COMMAND_IGNORE;
	double vccInt;
	double vccAux;
	double vBRAM;

	if(command == "xadc read")
	{
		ReadVoltages(&vccInt, &vccAux, &vBRAM);

		std::cout << "Current Temperature = " << ReadTemperatureF() << DEGREE_SYMBOL "F\n"
			<< "Min Temperature = " << ReadMinTemperatureF() << DEGREE_SYMBOL "F\n"
			<< "Max Temperature = " << ReadMaxTemperatureF() << DEGREE_SYMBOL "F\n\n"
			<< "Current VCCINT = " << vccInt << "v\n"
			<< "Current VCCAUX = " << vccAux << "v\n"
			<< "Current VBRAM = " << vBRAM << "v\n";

		commandResult = COMMAND_OK;
	}

	return commandResult;
}


