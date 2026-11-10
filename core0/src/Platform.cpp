/*
 * Platform.c
 *
 * Platform specific procedures.
 *
 * Author Jacob A Psimos 2025
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "Platform.h"
#include "Helpers.h"
#include <cstdint>

using namespace Helpers;

// ❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖
// ❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖ MMU        ❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖
// ❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖❖


static inline void dsb(void)  { asm volatile("dsb\n" : : : "memory"); }
static inline void isb(void)  { asm volatile("isb\n" : : : "memory"); }


/*
 * map_section_alias:
 *  - phys_addr and virt_addr must be 1MB-aligned
 *  - size_bytes must be multiple of 1MB
 *
 * This copies the L1 section descriptors from the identity-mapped region
 * (phys) to the target virtual addresses (virt).
 */
bool map_section_alias(uint32_t phys_addr, uint32_t virt_addr, uint32_t size_bytes)
{
	uint32_t n_sections;
	uint32_t ttbr0;
	uint32_t ttb_base;
	volatile uint32_t *ttb;
	uint32_t src_idx;
	uint32_t dst_idx;
	uint32_t entry;
	uint32_t i;

	phys_addr = ALIGN(phys_addr, 0x100000, ALIGN_LOW);
	virt_addr = ALIGN(virt_addr, 0x100000, ALIGN_LOW);
	size_bytes = ALIGN(size_bytes, 0x100000, ALIGN_HIGH);

    n_sections = size_bytes >> 20;

    asm volatile("mrc p15, 0, %0, c2, c0, 0" : "=r"(ttbr0));

    // TTBR0 bottom bits may hold flags; base is top-aligned to 16KB (mask low 14 bits)
    ttb_base = ttbr0 & (~0x3FFFU); // clear low 14 bits => 16KB alignment
    ttb = (volatile uint32_t *)ttb_base;

    src_idx = phys_addr >> 20;
    dst_idx = virt_addr >> 20;

    // Copy L1 descriptors
    for (i = 0; i < n_sections; ++i) {
        entry = ttb[src_idx + i];
        if (entry == 0) {
            // source not mapped -> fail
            return false;
        }
        ttb[dst_idx + i] = entry;
    }

    // Make sure changes reach memory and CPU uses new table
    dsb();

    i = 0;
    asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r"(i) : "memory");

    dsb();
    isb();

    return true;
}


#ifdef __cplusplus
}
#endif
