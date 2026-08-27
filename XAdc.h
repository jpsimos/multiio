/*
 * XAdc.h
 *
 *  Created on: Feb 22, 2025
 *      Author: jacob a psimos
 */

#ifndef SRC_XADC_H_
#define SRC_XADC_H_

#include "xadcps.h"
#include "Helpers.h"
#include "Middleware.h"

using namespace Helpers;

class XAdc : public Middleware
{
private:
	inline static XAdc* sm_singletonPtr = nullptr;

public:
	static XAdc* CreateInstance(const unsigned short deviceId, const unsigned char intrId);
	static XAdc* GetInstance();

private:
	XAdc(const unsigned short deviceId, const unsigned char intrId);
	virtual ~XAdc();
	static void HandleIsr(XAdc* adcPtr);

public:
	double ReadTemperatureF();
	double ReadMinTemperatureF();
	double ReadMaxTemperatureF();
	void ReadVoltages(
		double* outVccInt,
		double* outVccAux,
		double* outVBRAM
	);
	void ReadMinVoltages(
		double* outVccInt,
		double* outVccAux,
		double* outVBRAM
	);
	void ReadMaxVoltages(
		double* outVccInt,
		double* outVccAux,
		double* outVBRAM
	);
	virtual int ProcessCommand(const std::string& command);

private:
	const unsigned char m_intrId;
	XAdcPs_Config* m_adcConfig;
	XAdcPs m_adc;
};

#endif /* SRC_XADC_H_ */
