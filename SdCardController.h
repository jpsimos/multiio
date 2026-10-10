/*
 * SDCardController.h
 *
 *  Created on: Jan 16, 2025
 *      Author: jacob a psimos
 */

#ifndef SRC_SDCARDCONTROLLER_H_
#define SRC_SDCARDCONTROLLER_H_

#include <string>
#include "xsdps.h"
#include "Middleware.h"
#include "Helpers.h"

using namespace Helpers;

class SdCardController: public Middleware
{
public:
	struct SD_CID
	{
		uint8_t manufId;
		uint16_t oemId;
		uint8_t productName[5];
		uint8_t productRev;
		uint32_t productSerialNo;
		uint16_t manufDate;
		uint8_t crc7;
	} __attribute__((packed));

public:
	static SdCardController* CreateInstance(const unsigned int sdDeviceId);
	static SdCardController* GetInstance();

private:
	SdCardController(const unsigned int sdDeviceId);
	virtual ~SdCardController();
	static DWORD GetFATTimeCallback(void* arg);
	virtual int ProcessCommand(const std::string& command);

public:
	bool Mount();
	bool Unmount();
	bool FileExists(const std::string& fileName);
	size_t GetFileSize(const std::string& fileName);
	size_t ReadFile(const std::string& fileName, void* data, const size_t numBytes);
	bool AppendFile(const std::string& fileName, const void* data, const size_t numBytes);
	bool DeleteFile(const std::string& fileName);
	bool RenameFile(const std::string& fileName, const std::string& newFileName);

private:
	const unsigned int m_sdDeviceId;
	bool m_isMounted;
	XSdPs* m_sdPtr;
	alignas(32) FATFS m_fat;
};

#endif /* SRC_SDCARDCONTROLLER_H_ */
