/*
 * SPIFlash.cpp
 *
 *  Created on: Feb 22, 2025
 *      Author: jacob a psimos
 */

#include <iostream>
#include <string>
#include "xparameters.h"
#include "Helpers.h"
#include "SpiFlash.h"

using namespace Helpers;

SpiFlash* SpiFlash::CreateInstance(const unsigned int psDeviceId)
{
	if(nullptr == SpiFlash::sm_spiFlash)
	{
		SpiFlash::sm_spiFlash = new SpiFlash(psDeviceId);
	}
	return SpiFlash::sm_spiFlash;
}

SpiFlash* SpiFlash::GetInstance()
{
	if(nullptr == SpiFlash::sm_spiFlash)
	{
		RUNTIME_ERROR("Must call CreateInstance first");
	}
	return SpiFlash::sm_spiFlash;
}


SpiFlash::SpiFlash(const unsigned int psDeviceId) :
	Middleware(Middleware::AbstractType::QSPI_PS_FLASH_CONTROLLER),
	m_spiPsConfig(XQspiPs_LookupConfig(psDeviceId)),
	m_spiPs()
{
	int result;

	result = XQspiPs_CfgInitialize(&m_spiPs, m_spiPsConfig, m_spiPsConfig->BaseAddress);

	if(XST_SUCCESS != result)
	{
		RUNTIME_ERROR("XQspiPs_CfgInitialize = %x", result);
	}

	XQspiPs_SetOptions(&m_spiPs, XQSPIPS_MANUAL_START_OPTION | XQSPIPS_HOLD_B_DRIVE_OPTION);
	XQspiPs_SetClkPrescaler(&m_spiPs, XQSPIPS_CLK_PRESCALE_4);
	XQspiPs_SetSlaveSelect(&m_spiPs);

	// Winbond W25Q128 : 0xef4018
	m_capacityInBytes = 0x1000000U;
	m_pageSizeInBytes = 0x100U;
	m_sectorSizeInBytes = 0x1000U;
}

int SpiFlash::ProcessCommand(const std::string& command)
{
	int commandResult = COMMAND_IGNORE;
	unsigned int address;
	unsigned int numBytes;

	if(Text::StringBeginsWith(command, "nor "))
	{
		if(command == "nor status")
		{
			std::cout << "ID = 0x" << std::hex << GetID() << "\n"
					<< "status[1] = 0x" << std::hex << (int)GetStatusRegister(1) << "\n"
					<< "status[2] = 0x" << std::hex << (int)GetStatusRegister(2) << "\n"
					<< "status[3] = 0x" << std::hex << (int)GetStatusRegister(3) << "\n";

			commandResult = COMMAND_OK;
		}
		else if(command == "nor wp")
		{
			commandResult = WriteProtect(!IsWriteProtected()) ? COMMAND_OK : COMMAND_ERROR;
		}
		else if(command == "nor wipe")
		{
			commandResult = Erase(0, SpiFlash::EraseType::CHIP) ? COMMAND_OK : COMMAND_ERROR;
		}
		else if(2 == std::sscanf(command.c_str(), "nor erase %x %x", &address, &numBytes)
			&& command.length() == numBytes)
		{
			std::cout << "Erasing at 0x" << std::hex << address << " to 0x"
					<< std::hex << (address + numBytes) << "...\n";

			commandResult = EraseArea(address, numBytes) ? COMMAND_OK : COMMAND_ERROR;
		}
		else if(2 == std::sscanf(command.c_str(), "nor read %x %x", &address, &numBytes)
			&& command.length() == numBytes)
		{
			std::unique_ptr<unsigned char[]> readBuffer(new unsigned char[numBytes]);
			const unsigned int numBytesRead = Read(address, readBuffer.get(), numBytes);

			for(unsigned int a = 0; a < numBytesRead; a += m_sectorSizeInBytes)
			{
				Text::PrintHexTable(&readBuffer.get()[a], std::min(m_sectorSizeInBytes, numBytesRead - a), address + a);
			}

			std::cout << "checksum = 0x" << std::hex << Checksum::CRC32::Calculate(readBuffer.get(), numBytesRead) << "\n";

			commandResult = numBytes == numBytesRead ? COMMAND_OK : COMMAND_ERROR;
		}
		else if(2 == std::sscanf(command.c_str(), "nor crc %x %x", &address, &numBytes)
			&& command.length() == numBytes)
		{
			std::unique_ptr<unsigned char[]> readBuffer(new unsigned char[numBytes]);
			const unsigned int numBytesRead = Read(address, readBuffer.get(), numBytes);
			const uint32_t checksum = Checksum::CRC32::Calculate(readBuffer.get(), numBytesRead);

			std::cout << Text::Stringf("address = 0x%x, numBytes = 0x%x, numBytesRead = 0x%x, checksum = 0x%x\n",
					address, numBytes, numBytesRead, checksum);

			commandResult = numBytes == numBytesRead ? COMMAND_OK : COMMAND_ERROR;
		}
		else if(command == "nor test")
		{
			const unsigned int testBuffer1Size = Random::Random<unsigned int>(0U, m_sectorSizeInBytes * 16U);
			std::unique_ptr<unsigned char[]> testBuffer1(new unsigned char[testBuffer1Size]);
			std::unique_ptr<unsigned char[]> testBuffer1In(new unsigned char[testBuffer1Size]);

			for(unsigned int i = 0; i < testBuffer1Size; i++)
			{
				testBuffer1.get()[i] = Random::Random<unsigned char>(0, 255);
			}

			address = Random::Random<unsigned int>(0, m_capacityInBytes - (m_sectorSizeInBytes * 32));

			std::cout << "Test Address: 0x" << std::hex << address
					<< "\nTest Pattern 1 Size: 0x" << std::hex << testBuffer1Size << '\n';

			numBytes = Write(address, testBuffer1.get(), testBuffer1Size);

			std::cout << "Write TestPattern1: 0x" << std::hex << numBytes << '\n';

			numBytes = Read(address, testBuffer1In.get(), testBuffer1Size);

			std::cout << "Read TestPattern1: 0x" << std::hex << numBytes  << '\n';

			std::cout << "Verify Test Pattern 1: "
				<< (!std::memcmp(testBuffer1.get(), testBuffer1In.get(), testBuffer1Size) ? "Pass" : "Fail") << '\n';

			commandResult = COMMAND_OK;
		}
	}

	return commandResult;
}

unsigned char SpiFlash::GetStatusRegister(const unsigned char statusRegNum)
{
	s32 result;
	u8 data[2];

	result = XST_SUCCESS;
	data[1] = 0;

	switch(statusRegNum)
	{
		case 1:
			data[0] = 0x05U;
			break;
		case 2:
			data[0] = 0x35U;
			break;
		case 3:
			data[0] = 0x15U;
			break;
		default:
			result = XST_INVALID_PARAM;
			break;
	}

	if(XST_SUCCESS == result)
	{
		result = XQspiPs_PolledTransfer(&m_spiPs, data, data, 2);
	}

	return data[1];
}

unsigned int SpiFlash::GetID()
{
	s32 result;
	u8 out[4];

	out[0] = 0x9F;
	out[1] = 0;
	out[2] = 0;
	out[3] = 0;

	if(Wake())
	{
		result = XQspiPs_PolledTransfer(&m_spiPs, &out[0], &out[0], 4);

		if(XST_SUCCESS != result)
		{
			DEBUG_LOG("result = %x", result);
		}
	}

	return (((unsigned int)out[1] << 16) | ((unsigned int)out[2] << 8) | (unsigned int)out[3]);
}

bool SpiFlash::Wake()
{
	s32 result;
	u8 out[1];

	out[0] = 0xAB;

	result = XQspiPs_PolledTransfer(&m_spiPs, out,  NULL,  1);

	return static_cast<bool>(XST_SUCCESS == result);
}

bool SpiFlash::IsBusy()
{
	return static_cast<bool>(GetStatusRegister(1) & 0x01);
}

bool SpiFlash::WaitBusy(const unsigned int micros)
{
	Time::Stopwatch timeout;

	do
	{
		if(!IsBusy())
		{
			return false;
		}
		else
		{
			usleep(1000);
		}
	}
	while(timeout < micros);

	return IsBusy();
}

bool SpiFlash::IsWriteProtected()
{
	return static_cast<bool>(0 != (GetStatusRegister(1) & 0x1C));
}

bool SpiFlash::WriteEnable(const bool enable)
{
	s32 result;
	u8 out[1];

	result = XST_FAILURE;

	out[0] = enable ? 0x06 : 0x04;

	if(Wake())
	{
		result = XQspiPs_PolledTransfer(&m_spiPs, out, NULL, 1);
	}

	return static_cast<bool>(XST_SUCCESS == result);
}

bool SpiFlash::WriteProtect(const bool protect)
{
	s32 result;
	u8 out[2];

	result = XST_FAILURE;

	out[0] = 0x01;
	out[1] = protect ? 0x1C : 0x00;

	if(WriteEnable())
	{
		result = XQspiPs_PolledTransfer(&m_spiPs, out, NULL, 2);
	}

	return static_cast<bool>(XST_SUCCESS == result);
}

unsigned int SpiFlash::Write(unsigned int address, const void* data, const unsigned int numBytes)
{
	// ⧖⧖⧖   Constants
	const unsigned char* dataPtr = static_cast<const unsigned char*>(data);
	const unsigned int beginSectorAddress = ALIGN(address, m_sectorSizeInBytes, ALIGN_LOW);
	const unsigned int endSectorAddress = ALIGN((address + numBytes), m_sectorSizeInBytes, ALIGN_HIGH);
	const unsigned int dataBufferSize = endSectorAddress - beginSectorAddress;
	const unsigned int dataBufferInsertionOffset = address - beginSectorAddress;

	// ⧖⧖⧖   Variables
	bool hadError = false;
	unsigned int numBytesRead = 0;
	unsigned int numBytesWrote = 0;
	unsigned int dataBufferOffset = 0;
	s32 spiResult = 0;

	// ⧖⧖⧖   Buffers
	std::unique_ptr<unsigned char[]> dataBuffer;
	std::unique_ptr<unsigned char[]> spiWritePageBuffer;

	if(0 == numBytes || address + numBytes > m_capacityInBytes)
	{
		DEBUG_ERROR("Invalid length.");
		hadError = true;
	}

	if(!hadError)
	{
		// ⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖
		// ⧖⧖⧖   Read then erase all existing data from flash between boundary sectors,
		// ⧖⧖⧖   then merge user data into the buffer at it's offset from the beginning sector address
		// ⧖⧖⧖   then program the flash page by page
		// ⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖⧖

		dataBuffer.reset(new unsigned char[dataBufferSize]);
		spiWritePageBuffer.reset(new unsigned char[m_pageSizeInBytes + 4]);

		numBytesRead = Read(beginSectorAddress, dataBuffer.get(), dataBufferSize);

		if(dataBufferSize == numBytesRead)
		{
			// ⧖⧖⧖    Merge user data into buffer offset from the beginning sector address
			std::memcpy(&(dataBuffer.get()[dataBufferInsertionOffset]), dataPtr, numBytes);

			// ⧖⧖⧖    Erase the sectors in flash
			for(unsigned int eraseAddress = beginSectorAddress; eraseAddress < endSectorAddress; eraseAddress += m_sectorSizeInBytes)
			{
				if(!Erase(eraseAddress, SpiFlash::EraseType::SECTOR))
				{
					DEBUG_ERROR("unable to erase sector at 0x%x", eraseAddress);
					hadError = true;
				}
			}

			if(!hadError)
			{
				dataBufferOffset = 0;

				// ⧖⧖⧖    Loop over the data buffer and program the flash page by page
				for(unsigned int curAddress = beginSectorAddress; curAddress < endSectorAddress; curAddress += m_pageSizeInBytes)
				{
					unsigned char* spiWritePageBufferPtr = spiWritePageBuffer.get();

					spiWritePageBufferPtr[0] = 0x02;
					spiWritePageBufferPtr[1] = (unsigned char)((curAddress >> 16) & 0xFF);
					spiWritePageBufferPtr[2] = (unsigned char)((curAddress >> 8) & 0xFF);
					spiWritePageBufferPtr[3] = (unsigned char)(curAddress & 0xFF);
					std::memcpy(&spiWritePageBufferPtr[4], &(dataBuffer.get()[dataBufferOffset]), m_pageSizeInBytes);
					dataBufferOffset += m_pageSizeInBytes;

					if(!WriteEnable(true))
					{
						DEBUG_ERROR("WriteEnable failed");
						hadError = true;
						break;
					}

					spiResult = XQspiPs_PolledTransfer(&m_spiPs, spiWritePageBufferPtr, NULL, m_pageSizeInBytes + 4);

					if(XST_SUCCESS != spiResult)
					{
						DEBUG_ERROR("XQspiPs_PolledTransfer = 0x%x", spiResult);
						hadError = true;
						break;
					}

					if(WaitBusy(10000))
					{
						DEBUG_ERROR("Busy timeout");
						hadError = true;
						break;
					}

					numBytesWrote += m_pageSizeInBytes;
				}
			}
		}
		else
		{
			DEBUG_ERROR("numBytesRead(%u) != %u", numBytesRead, dataBufferSize);
			hadError = true;
		}
	}

	return numBytesWrote;
}

unsigned int SpiFlash::Read(unsigned int address, void* data, const unsigned int numBytes)
{
	unsigned char* dataPtr = static_cast<unsigned char*>(data);
	std::unique_ptr<unsigned char[]> spiBuffer(new unsigned char[numBytes + 4]);
	unsigned char* buf = spiBuffer.get();
	s32 result;

	if(address + numBytes > m_capacityInBytes)
	{
		DEBUG_ERROR("capacity out of range");
		return 0;
	}

	buf[0] = 0x03U;
	buf[1] = static_cast<unsigned char>((address >> 16) & 0xFF);
	buf[2] = static_cast<unsigned char>((address >> 8) & 0xFF);
	buf[3] = static_cast<unsigned char>(address & 0xFF);

	if(!Wake())
	{
		DEBUG_ERROR("no wake");
		return 0;
	}

	result = XQspiPs_PolledTransfer(&m_spiPs, &buf[0], &buf[0], numBytes + 4);

	if(XST_SUCCESS != result)
	{
		DEBUG_ERROR("XQspiPs_PolledTransfer = %x", result);
		return 0;
	}

	if(NULL != dataPtr)
	{
		std::memcpy(&dataPtr[0], &buf[4], numBytes);
	}

	return numBytes;
}

/***
 * @return true if there was not an error
 */
bool SpiFlash::Erase(unsigned int address, const SpiFlash::EraseType eraseType)
{
	bool hadError = false;
	unsigned int alignedAddress = 0;
	unsigned int outLength = 0;
	s32 result;
	u8 out[4];

	switch(eraseType)
	{
	case SpiFlash::EraseType::SECTOR:
		out[0] = 0x20U;
		alignedAddress = ALIGN(address, m_sectorSizeInBytes, ALIGN_LOW);
		outLength = 4;
		break;
	case SpiFlash::EraseType::SECTOR_32K:
		out[0] = 0x52U;
		alignedAddress = ALIGN(address, 32768U, ALIGN_LOW);
		outLength = 4;
		break;
	case SpiFlash::EraseType::SECTOR_64K:
		out[0] = 0xD8U;
		alignedAddress = ALIGN(address, 65536U, ALIGN_LOW);
		outLength = 4;
		break;
	case SpiFlash::EraseType::CHIP:
		out[0] = 0xC7U;
		outLength = 1;
		break;
	}

	if(outLength > 1)
	{
		if(alignedAddress != address)
		{
			DEBUG_LOG("Provided address 0x%x aligned to 0x%x", address, alignedAddress);
		}

		out[1] = static_cast<unsigned char>((alignedAddress >> 16) & 0xFF);
		out[2] = static_cast<unsigned char>((alignedAddress >> 8) & 0xFF);
		out[3] = static_cast<unsigned char>(alignedAddress & 0xFF);
	}

	if(!WriteEnable())
	{
		DEBUG_ERROR("no write enable");
		hadError = true;
	}

	if(!hadError)
	{
		result = XQspiPs_PolledTransfer(&m_spiPs, out, NULL, outLength);

		if(XST_SUCCESS != result)
		{
			DEBUG_ERROR("XQspiPs_PolledTransfer = %x", result);
			hadError = true;
		}

		if(!hadError && WaitBusy(60000000))
		{
			DEBUG_ERROR("timeout busy");
			hadError = true;
		}
	}

	return !hadError;
}

/***
 * @return true if there was not an error
 */
bool SpiFlash::EraseArea(const unsigned int address, const unsigned int numBytes)
{
	const unsigned int beginSectorAddress = ALIGN(address, m_sectorSizeInBytes, ALIGN_LOW);
	const unsigned int endSectorAddress = ALIGN((address + numBytes), m_sectorSizeInBytes, ALIGN_HIGH);
	bool hadError = false;

	for(unsigned int eraseAddress = beginSectorAddress; eraseAddress < endSectorAddress; eraseAddress += m_sectorSizeInBytes)
	{
		hadError = !Erase(eraseAddress, SpiFlash::EraseType::SECTOR);

		if(hadError)
		{
			DEBUG_ERROR("Error erasing sector at 0x%x", eraseAddress);
			break;
		}
	}

	return !hadError;
}
