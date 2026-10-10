/*
 * SDCardController.cpp
 *
 *  Created on: Jan 16, 2025
 *      Author: jacob a psimos
 */

#include <iostream>
#include "Helpers.h"
#include "xsdps.h"
#include "xsdps_core.h"
#include "ff.h"
#include "ffconf.h"
#include "diskio.h"
#include "SdCardController.h"
#include "HwClock.h"

using namespace Helpers;

static SdCardController* sdCardControllerPtr = NULL;

SdCardController* SdCardController::CreateInstance(const unsigned int sdDeviceId)
{
	if(NULL == sdCardControllerPtr)
	{
		sdCardControllerPtr = new SdCardController(sdDeviceId);
	}
	return sdCardControllerPtr;
}

SdCardController* SdCardController::GetInstance()
{
	if(NULL == sdCardControllerPtr)
	{
		RUNTIME_ERROR("Must call CreateInstance first.");
	}
	return sdCardControllerPtr;
}

SdCardController::SdCardController(const unsigned int sdDeviceId) :
	Middleware(Middleware::AbstractType::SD_CARD_CONTROLLER),
	m_sdDeviceId(sdDeviceId),
	m_isMounted(false),
	m_sdPtr(disk_get_driver(sdDeviceId)),
	m_fat()
{
	disk_set_fattime_callback(SdCardController::GetFATTimeCallback, this);
	if(nullptr == m_sdPtr)
	{
		RUNTIME_ERROR("Invalid SD interface");
	}

	Mount();
}

SdCardController::~SdCardController()
{
	Unmount();
}

DWORD SdCardController::GetFATTimeCallback(void* arg)
{
	HwClock::HwClock_DT dateTime;
	DWORD fatDateTime;

	if(HwClock::GetInstance()->GetDateTime(dateTime))
	{
		fatDateTime = ((dateTime.Year < 1980 ? dateTime.Year + 2000 : dateTime.Year) - 1980) << 25;
		fatDateTime |= dateTime.Month << 21;
		fatDateTime |= dateTime.Day << 16;
		fatDateTime |= dateTime.Hour << 11;
		fatDateTime |= dateTime.Min << 5;
		fatDateTime |= dateTime.Sec >> 1;
	}
	else
	{
		fatDateTime = (1980U << 25U)
			| (1 << 21)
			| (1 << 16)
			| (0 << 11)
			| (0 << 5)
			| (0 >> 1);
	}

	return fatDateTime;
}

int SdCardController::ProcessCommand(const std::string& command)
{
	int commandResult = COMMAND_IGNORE;
	FRESULT fresult;
	FILINFO finfo;
	DIR dirHandle;

	if(Text::StringBeginsWith(command, "sd "))
	{
		if(command == "sd status")
		{
			const unsigned long long capacity = static_cast<unsigned long long>(m_sdPtr->SectorCount) * 512ULL;
			const unsigned int sdStatus = XSdPs_GetPresentStatusReg(m_sdPtr->Config.BaseAddress);
			SD_CID cid;

			std::cout << "Card Type = " << std::dec << (unsigned int)m_sdPtr->CardType << "\n"
				<< "Mode = " << std::dec << m_sdPtr->Mode << "\n"
				<< "Bus Speed = " << std::dec << m_sdPtr->BusSpeed << "\n"
				<< "Sector Count = " << std::dec << m_sdPtr->SectorCount << "\n"
				<< "Block Size = " << std::dec << 512ULL << "\n"
				<< "Capacity = " << std::dec << capacity << "\n"
				<< "Mounted = " << m_isMounted << "\n"
				<< "Controller Status = 0x" << std::hex << sdStatus << "\n";

			std::cout << "SD CSD = 0x" << std::hex << (unsigned int)m_sdPtr->CardSpecData[0]
				<< ", 0x" << std::hex << (unsigned int)m_sdPtr->CardSpecData[1]
				<< ", 0x" << std::hex << (unsigned int)m_sdPtr->CardSpecData[2]
				<< ", 0x" << std::hex << (unsigned int)m_sdPtr->CardSpecData[3]
				<< "\n";

			for(unsigned int i = 0; i < sizeof(SD_CID); i++)
			{
				((unsigned char*)&cid)[sizeof(SD_CID) - (i + 1)] = ((unsigned char*)m_sdPtr->CardID)[i];
			}

			std::cout << "SD CID {\n"
				<< "\tManufacturer ID = 0x" << std::hex << (unsigned int)cid.manufId << "\n"
				<< "\tOEM ID = 0x" << std::hex << (unsigned int)cid.oemId << "\n"
				<< "\tProduct Name = 0x" << std::hex << (unsigned int)cid.productName[0]
				<< ", 0x" << std::hex << (unsigned int)cid.productName[1] << ", 0x" << std::hex << (unsigned int)cid.productName[2]
				<< ", 0x" << std::hex << (unsigned int)cid.productName[3] << ", 0x" << std::hex << (unsigned int)cid.productName[4] << "\n"
				<< "\tProduct Revision = 0x" << std::hex << (unsigned int)cid.productRev << "\n"
				<< "\tProduct Serial Number = 0x" << std::hex << (unsigned int)cid.productSerialNo << "\n"
				<< "\tManufacture Date = 0x" << std::hex << (unsigned int)cid.manufDate << "\n"
				<< "\tCRC7 = 0x" << std::hex << (unsigned int)cid.crc7 << "\n"
				<< "}\n";

			commandResult = COMMAND_OK;
		}
		else if(command == "sd mount")
		{
			commandResult = Mount() ? COMMAND_OK : COMMAND_ERROR;
		}
		else if(command == "sd unmount")
		{
			commandResult = Unmount() ? COMMAND_OK : COMMAND_ERROR;
		}
		else if(Text::StringBeginsWith(command, "sd ls"))
		{
			commandResult = COMMAND_ERROR;

			if(m_isMounted)
			{
				const char* path = static_cast<const char*>(command.length() >= 8 ? &command.c_str()[6] : "/");

				fresult = f_opendir(&dirHandle, path);

				if(FR_OK == fresult)
				{
					commandResult = COMMAND_OK;
				}

				do
				{
					finfo.fname[0] = '\0';

					if(FR_OK == fresult)
					{
						fresult = f_readdir(&dirHandle, &finfo);

						if(FR_OK == fresult && '\0' != finfo.fname[0])
						{
							const unsigned int year = 1980 + ((finfo.fdate & 0xFE00) >> 9);
							const unsigned int month = (finfo.fdate & 0x1E0) >> 5;
							const unsigned int day = finfo.fdate & 0x1F;
							const unsigned int hour = (finfo.ftime & 0xF800) >> 11;
							const unsigned int minute = (finfo.ftime & 0x7E0) >> 5;
							const unsigned int second = (finfo.ftime & 0x1F) << 1;

							std::ios::fmtflags flags = std::cout.flags();

							std::cout << std::setfill(' ') << std::setw(11) << std::left
									<< Text::Stringf("%u/%u/%u", month, day, year)
									<< Text::Stringf("%02u:%02u:%02u", hour, minute, second);

							if(~finfo.fattrib & AM_DIR)
							{
								std::cout << std::setw(12) << std::right << std::dec << (unsigned long)finfo.fsize;
							}
							else
							{
								std::cout << std::setw(12) << std::right << ' ';
							}

							std::cout.flags(flags);

							std::cout << "   ";

							if(finfo.fattrib & AM_DIR)
							{
								std::cout << ANSI_LIGHT_BLUE << finfo.fname << ANSI_RESET << '\n';
							}
							else
							{
								std::cout << finfo.fname << '\n';
							}
						}
					}
				}
				while('\0' != finfo.fname[0]);

				f_closedir(&dirHandle);
			}
		}
	}

	return commandResult;
}

bool SdCardController::Mount()
{
	FRESULT result;

	if(!m_isMounted)
	{
		result = f_mount(&m_fat, "0:/", 1);

		if(FR_OK == result)
		{
			m_isMounted = true;
		}
	}

	return m_isMounted;
}

bool SdCardController::Unmount()
{
	FRESULT result;

	if(m_isMounted)
	{
		result = f_unmount("0:/");

		if(FR_OK == result)
		{
			m_isMounted = false;
		}
	}

	return static_cast<bool>(false == m_isMounted);
}

bool SdCardController::FileExists(const std::string& fileName)
{
	FRESULT res;
	FILINFO fi;

	if(m_isMounted && fileName.length())
	{
		res = f_stat(fileName.c_str(), &fi);

		if(FR_OK == res)
		{
			return true;
		}
	}
	return false;
}

size_t SdCardController::GetFileSize(const std::string& fileName)
{
	FRESULT res;
	FILINFO fi;

	fi.fsize = 0;

	if(m_isMounted && fileName.length())
	{
		res = f_stat(fileName.c_str(), &fi);

		if(FR_OK != res)
		{
			fi.fsize = 0;
		}
	}

	return static_cast<size_t>(fi.fsize);
}

size_t SdCardController::ReadFile(const std::string& fileName, void* data, const size_t numBytes)
{
	unsigned char* dataPtr = reinterpret_cast<unsigned char*>(data);
	FIL fil;
	FRESULT res;
	size_t numBytesRead;
	size_t filSize;

	numBytesRead = 0;

	if(m_isMounted)
	{
		filSize = GetFileSize(fileName);

		res = f_open(&fil, fileName.c_str(), FA_READ);

		if(FR_OK == res)
		{
			numBytesRead = 0;

			res = f_read(&fil, dataPtr, (UINT)filSize, (UINT*)&numBytesRead);

			if(FR_OK != res)
			{
				DEBUG_LOG("f_read = %x", (int)res);
			}

			res = f_close(&fil);

			if(FR_OK != res)
			{
				DEBUG_LOG("f_close = %x", (int)res);
			}
		}
	}

	return numBytesRead;
}

bool SdCardController::AppendFile(const std::string& fileName, const void* data, const size_t numBytes)
{
	bool success = false;
	FIL fil;
	FRESULT res;
	UINT totalBytesWritten;

	if(m_isMounted)
	{
		res = f_open(&fil, fileName.c_str(), FA_OPEN_APPEND | FA_WRITE | FA_OPEN_ALWAYS);

		if(FR_OK == res)
		{
			totalBytesWritten = 0;

			res = f_write(&fil, data, (UINT)numBytes, &totalBytesWritten);

			if(FR_OK == res && numBytes == totalBytesWritten)
			{
				success = true;
			}

			res = f_close(&fil);
		}
	}

	return success;
}

bool SdCardController::DeleteFile(const std::string& fileName)
{
	FRESULT res = FR_NO_FILESYSTEM;

	if(m_isMounted)
	{
		res = f_unlink(fileName.c_str());
	}

	return static_cast<bool>(FR_OK == res);
}

bool SdCardController::RenameFile(const std::string& fileName, const std::string& newFileName)
{
	FRESULT res = FR_NO_FILESYSTEM;

	if(m_isMounted && !fileName.empty() && !newFileName.empty())
	{
		res = f_rename(fileName.c_str(), newFileName.c_str());
	}

	return static_cast<bool>(FR_OK == res);
}


