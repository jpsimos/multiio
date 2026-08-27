#ifndef __HELPERS_H
#define __HELPERS_H

/*
	Helpers.h - A utility class that is designed to make the life of engineer easier.
	Author: Jacob Psimos
*/

#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cstdint>
#include <limits>
#include <random>
#include <functional>
#include <sys/time.h>
#include "xparameters.h"
#include "xil_cache.h"
#include "xtime_l.h"
#include "sleep.h"

#define COUNTS_PER_MICROSECOND                   (((long long)XPAR_CPU_CORTEXA9_CORE_CLOCK_FREQ_HZ / 2LL) / 1000000LL)
#define CRC32_DEFAULT_BIT_REFLECTED_POLYNOMIAL   0xEDB88320
#define CRC32_DEFAULT                            0xFFFFFFFF
#define CRC32_XOR                                0xFFFFFFFF
#define CRC16_DEFAULT_BIT_REFLECTED_POLYNOMIAL   0xA001
#define CRC16_DEFAULT                            0xFFFF
#define CRC16_XOR                                0x0000

#define nop()                               __asm__ __volatile__("nop")
#define sev()                               __asm__ __volatile__("sev" : : : "memory")
#define ISPOWEROF2(x)                       !(((x) != 0) && ((x) & ((x) - 1)))
#define ISODD(x)                            !!((x) & 1)
#define ISEVEN(x)                           !!((~(x)) & 1)
#define ALIGN_LOW(val, alignval) \
	(((val) + (alignval)) & (((alignval) - 1) - (alignval)))
#define ALIGN_HIGH(val, alignval) \
	(((val) + (alignval)) & (((alignval) - 1)))

#define DEBUG_LOG(fmt, ...) do { \
		std::cout << Text::Stringf(ANSI_YELLOW "[%u,%.8lf] %s:%d %s " ANSI_RESET fmt "\n", \
			XPAR_CPU_ID, Time::Seconds(), __BASE_FILE__, __LINE__, __func__, ##__VA_ARGS__); } while(0)
#define DEBUG_ERROR(fmt, ...) do { \
		std::cout << Text::Stringf(ANSI_RED "[%u,%.8lf] %s:%d %s " ANSI_RESET fmt "\n", \
			XPAR_CPU_ID, Time::Seconds(), __BASE_FILE__, __LINE__, __func__, ##__VA_ARGS__); } while(0)
#define RUNTIME_ERROR(fmt, ...) do { throw std::runtime_error( \
		Text::Stringf("[%u,%.8lf] %s:%d %s " fmt "\n", \
				XPAR_CPU_ID, Time::Seconds(), __BASE_FILE__, __LINE__, __func__, ##__VA_ARGS__)); } while(0)

#define ANSI_HARDRESET         "\e[>0n\e[>1n\e[>2n\e[>3n\e[>4n\e[>0m\e[>1m\e[>2m\e[>3m\e[>4m\x1b[H\x1b[0J\x1b[0m\x1b[0G"
#define ANSI_CLEARSCREEN       "\x1b[H\x1b[0J\x1b[0m"
#define ANSI_COLZERO           "\x1b[0G"
#define ANSI_ERASELINE         "\x1b[2K"
#define ANSI_ARROW_UP          "\x1b\x5b\x41"
#define ANSI_ARROW_DOWN        "\x1b\x5b\x42"
#define ANSI_ARROW_LEFT        "\x1b\x5b\x44"
#define ANSI_ARROW_RIGHT       "\x1b\x5b\x43"
#define ANSI_RESET             "\x1b\x5b\x30\x6d"
#define ANSI_DEFAULT           "\x1b\x5b\x33\x39\x6d"
#define ANSI_GREEN             "\x1b\x5b\x33\x32\x6d"
#define ANSI_RED               "\x1b\x5b\x33\x31\x6d"
#define ANSI_BLUE              "\x1b\x5b\x33\x34\x6d"
#define ANSI_MAGENTA           "\x1b\x5b\x33\x35\x6d"
#define ANSI_CYAN              "\x1b\x5b\x33\x36\x6d"
#define ANSI_YELLOW            "\x1b\x5b\x33\x33\x6d"
#define ANSI_LIGHT_GREEN       "\x1b\x5b\x30\x3b\x39\x32\x6d"
#define ANSI_LIGHT_RED         "\x1b\x5b\x30\x3b\x39\x31\x6d"
#define ANSI_LIGHT_BLUE        "\x1b\x5b\x30\x3b\x39\x34\x6d"
#define ANSI_LIGHT_MAGENTA     "\x1b\x5b\x30\x3b\x39\x35\x6d"
#define ANSI_LIGHT_CYAN        "\x1b\x5b\x30\x3b\x39\x36\x6d"
#define ANSI_LIGHT_YELLOW      "\x1b\x5b\x30\x3b\x39\x33\x6d"

#define DEGREE_SYMBOL          "\xc2\xb0"
#define MU_SYMBOL              "\xce\xbc"

namespace Helpers
{
	namespace Text
	{
		bool CStrEq(const char* __restrict__ a, const char* __restrict__ b);
		std::string Stringf(const char* __restrict__ const fmt, ...);
		std::string StringReplace(std::string str, const std::string& what, const std::string& with);
		std::vector<std::string> StringSplit(const std::string& str, const std::string& delim);
		std::string StringToLower(std::string str);
		std::string StringToUpper(std::string str);
		bool StringBeginsWith(const std::string& str, const std::string& begins);
		bool StringEndsWith(const std::string& str, const std::string& ends);
		std::string HexTable(const void* ptr, const unsigned int num, const unsigned int voffset = 0U);
		void PrintHexTable(const void* ptr, const unsigned int num, const unsigned int voffset = 0U);
	}

	namespace Random
	{
		extern std::random_device randomDevice;
		extern std::default_random_engine randomEngine;

		void SeedRandom(unsigned long s);

		template<typename IntType, typename = std::enable_if_t<std::is_integral_v<IntType>>>
		IntType Random(const IntType inclMin, const IntType inclMax)
		{
			std::uniform_int_distribution<IntType> uniformDist(inclMin, inclMax);
			return uniformDist(randomEngine);
		}
	}

	namespace Numeric
	{
		uint16_t BitReflect16(uint16_t val);
		uint32_t BitReflect32(uint32_t val);
		int BCDToDec(const int bcd);
		int DecToBCD(const int dec);
		bool IsDoubleEqual(const double a, const double b, const double epsilon = 0.000001);
	}

	namespace Checksum
	{
		namespace CRC32
		{
			uint32_t Calculate(const void* data, const size_t dataSizeBytes, const uint32_t polynomial = CRC32_DEFAULT_BIT_REFLECTED_POLYNOMIAL);
			uint32_t Recalculate(uint32_t crc, const void* data, const size_t dataSizeBytes, const uint32_t polynomial = CRC32_DEFAULT_BIT_REFLECTED_POLYNOMIAL);
		}
		namespace CRC16
		{
			uint16_t Calculate(const void* data, const size_t dataSizeBytes, const uint16_t polynomial = CRC16_DEFAULT_BIT_REFLECTED_POLYNOMIAL);
			uint16_t Recalculate(uint16_t crc, const void* data, const size_t dataSizeBytes, const uint16_t polynomial = CRC16_DEFAULT_BIT_REFLECTED_POLYNOMIAL);
		}
	}

	namespace Time
	{
		extern "C" int _gettimeofday(struct timeval* tv, void* tz);
		unsigned int SecondsToTicks(const double seconds, const unsigned int frequency);
		long long Micros();
		long long Millis();
		double Seconds();
		double MicrosToSeconds(const long long micros);
		double MillisToSeconds(const long long millis);
		long long SecondsToMicros(double seconds);
		long long SecondsToMillis(const double seconds);
		void SleepMicros(const long long micros);
		void SleepMillis(const long long millis);
		void SleepSeconds(const double seconds);

		class Stopwatch
		{
		public:
			Stopwatch();
			Stopwatch(const int micros);
			Stopwatch(const unsigned int micros);
			Stopwatch(const long long micros);
			Stopwatch(const unsigned long long micros);
			Stopwatch(const double seconds);
			~Stopwatch() = default;
			void Reset();
			void Set(const long long micros);
			void Set(const unsigned long long micros);
			void Set(const int micros);
			void Set(const unsigned int micros);
			void Set(const double seconds);
			long long Get() const;
			long long GetMillis() const;
			double GetSeconds() const;
			void operator=(const long long& value);
			void operator=(const unsigned long long& value);
			void operator=(const int& value);
			void operator=(const unsigned int& value);
			void operator=(const double& value);
			bool operator>(const long long& value);
			bool operator>(const unsigned long long& value);
			bool operator>(const int& value);
			bool operator>(const unsigned int& value);
			bool operator>(const double& value);
			bool operator<(const long long& value);
			bool operator<(const unsigned long long& value);
			bool operator<(const int& value);
			bool operator<(const unsigned int& value);
			bool operator<(const double& value);
			bool operator>=(const long long& value);
			bool operator>=(const unsigned long long& value);
			bool operator>=(const int& value);
			bool operator>=(const unsigned int& value);
			bool operator>=(const double& value);
			bool operator<=(const long long& value);
			bool operator<=(const unsigned long long& value);
			bool operator<=(const int& value);
			bool operator<=(const unsigned int& value);
			bool operator<=(const double& value);
			bool operator==(const long long& value);
			bool operator==(const unsigned long long& value);
			bool operator==(const int& value);
			bool operator==(const unsigned int& value);
			bool operator==(const double& value);

			operator long long() const {
				return Get();
			}

			operator unsigned long long() const {
				return static_cast<unsigned long long>(Get());
			}

			operator int() const {
				return static_cast<int>(Get());
			}

			operator unsigned int() const {
				return static_cast<unsigned int>(Get());
			}

			operator double() const {
				return GetSeconds();
			}

		private:
			long long m_beganAt;
		};
	}

	namespace Crypto
	{
		class R4C
		{
			public:
				explicit R4C(const void* key, unsigned int keyLength);
				~R4C();
				void Crypt(void* data, const unsigned int dataLength);

			private:
				unsigned int x;
				unsigned int j;
				unsigned int y;
				unsigned int k;
				unsigned char keyState[256];
				unsigned char originalKey[16];
		};
	}
}

#endif
