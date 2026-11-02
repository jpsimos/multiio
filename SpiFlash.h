/*
 * SPIFlash.h
 *
 *  Created on: Feb 22, 2025
 *      Author: jacob
 */

#ifndef SRC_SPIFLASH_H_
#define SRC_SPIFLASH_H_

#include <string>
#include "Middleware.h"
#include "xqspips.h"
#include "Helpers.h"

class SpiFlash : public Middleware
{
public:
	enum class EraseType : unsigned int
	{
		SECTOR,
		SECTOR_32K,
		SECTOR_64K,
		CHIP
	};

private:
	inline static SpiFlash* sm_spiFlash = nullptr;

public:
	static SpiFlash* CreateInstance(const unsigned int psDeviceId);
	static SpiFlash* GetInstance();

private:
	SpiFlash(const unsigned int psDeviceId);
	virtual ~SpiFlash() {}

public:
	virtual int ProcessCommand(const std::string& command);
	unsigned char GetStatusRegister(const unsigned char statusRegNum);
	unsigned int GetID();
	bool IsWriteProtected();
	bool WriteProtect(const bool protect = true);
	unsigned int Write(unsigned int address, const void* data, const unsigned int numBytes);
	unsigned int Read(unsigned int address, void* data, const unsigned int numBytes);
	bool Erase(unsigned int address, const SpiFlash::EraseType eraseType = SpiFlash::EraseType::SECTOR);
	bool EraseArea(const unsigned int address, const unsigned int numBytes);

private:
	bool Wake();
	bool IsBusy();
	bool WaitBusy(const unsigned int micros);
	bool WriteEnable(const bool enable = true);

private:
	XQspiPs_Config* m_spiPsConfig;
	XQspiPs m_spiPs;
	unsigned int m_capacityInBytes;
	unsigned int m_pageSizeInBytes;
	unsigned int m_sectorSizeInBytes;
};

#endif /* SRC_SPIFLASH_H_ */
