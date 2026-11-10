/*
 * Platform.h
 *
 * Platform specific prototypes.
 *
 * Author Jacob A Psimos 2025
 */

#ifndef __PLATFORM_H
#define __PLATFORM_H
#include <cstdint>

#ifdef __cplusplus
extern "C"
{
#endif

bool map_section_alias(uint32_t phys_addr, uint32_t virt_addr, uint32_t size_bytes);

#ifdef __cplusplus
}
#endif

#endif
