
_STACK_SIZE = DEFINED(_STACK_SIZE) ? _STACK_SIZE : 0x20000;
_HEAP_SIZE = DEFINED(_HEAP_SIZE) ? _HEAP_SIZE : 0x400000;

_ABORT_STACK_SIZE = DEFINED(_ABORT_STACK_SIZE) ? _ABORT_STACK_SIZE : 1024;
_SUPERVISOR_STACK_SIZE = DEFINED(_SUPERVISOR_STACK_SIZE) ? _SUPERVISOR_STACK_SIZE : 2048;
_IRQ_STACK_SIZE = DEFINED(_IRQ_STACK_SIZE) ? _IRQ_STACK_SIZE : 1024;
_FIQ_STACK_SIZE = DEFINED(_FIQ_STACK_SIZE) ? _FIQ_STACK_SIZE : 1024;
_UNDEF_STACK_SIZE = DEFINED(_UNDEF_STACK_SIZE) ? _UNDEF_STACK_SIZE : 1024;


MEMORY
{
	cpu0_ddr : ORIGIN = 0x100000, LENGTH = 0x6400000
	cpu1_ddr : ORIGIN = 0x6500000, LENGTH = 0x6400000
	dma1_ddr : ORIGIN = 0xC900000, LENGTH = 0x100000
	dma2_ddr : ORIGIN = 0xCA00000, LENGTH = 0x100000
	dma3_ddr : ORIGIN = 0xCB00000, LENGTH = 0x100000
	dma4_ddr : ORIGIN = 0xCC00000, LENGTH = 0x100000
	queue1_ddr : ORIGIN = 0xCD00000, LENGTH = 0x200000
	queue2_ddr : ORIGIN = 0xCF00000, LENGTH = 0x200000
	queue3_ddr : ORIGIN = 0xD100000, LENGTH = 0x200000
	queue4_ddr : ORIGIN = 0xD300000, LENGTH = 0x200000
	unused_ddr : ORIGIN = 0xD500000, LENGTH = 0x2C00000
	vdisk_ddr : ORIGIN = 0x10100000, LENGTH = 0x10000000
	ps7_qspi_linear_0_memory_0 : ORIGIN = 0xFC000000, LENGTH = 0x1000000
	ocm_ram_0 : ORIGIN = 0x0, LENGTH = 0x30000
	ocm_ram_1 : ORIGIN = 0xFFFF0000, LENGTH = 0xFE00
}


ENTRY(_vector_table)

SECTIONS
{
_cpu0_ddr_start = ORIGIN(cpu0_ddr);
_cpu1_ddr_start = ORIGIN(cpu1_ddr);
_cpu1_ddr_end = ORIGIN(cpu1_ddr) + LENGTH(cpu1_ddr);
_dma1_ddr_start = ORIGIN(dma1_ddr);
_dma1_ddr_end = ORIGIN(dma1_ddr) + LENGTH(dma1_ddr);
_dma2_ddr_start = ORIGIN(dma2_ddr);
_dma2_ddr_end = ORIGIN(dma2_ddr) + LENGTH(dma2_ddr);
_queue1_ddr_start = ORIGIN(queue1_ddr);
_queue1_ddr_end = ORIGIN(queue1_ddr) + LENGTH(queue1_ddr);
_queue2_ddr_start = ORIGIN(queue2_ddr);
_queue2_ddr_end = ORIGIN(queue2_ddr) + LENGTH(queue2_ddr);
_queue3_ddr_start = ORIGIN(queue3_ddr);
_queue3_ddr_end = ORIGIN(queue3_ddr) + LENGTH(queue3_ddr);
_queue4_ddr_start = ORIGIN(queue4_ddr);
_queue4_ddr_end = ORIGIN(queue4_ddr) + LENGTH(queue4_ddr);
_unused_ddr_start = ORIGIN(unused_ddr);
_unused_ddr_end = ORIGIN(unused_ddr) + LENGTH(unused_ddr);
_vdisk_ddr_start = ORIGIN(vdisk_ddr);
_vdisk_ddr_end = ORIGIN(vdisk_ddr) + LENGTH(vdisk_ddr);
_ocm_ram_0_start = ORIGIN(ocm_ram_0);
_ocm_ram_0_end = ORIGIN(ocm_ram_0) + LENGTH(ocm_ram_0);
_ocm_ram_1_start = ORIGIN(ocm_ram_1);
_ocm_ram_1_end = ORIGIN(ocm_ram_1) + LENGTH(ocm_ram_1);

.text : {
   . = ALIGN(2048);
   KEEP (*(.vectors))
   *(.boot)
   *(.text)
   *(.text.*)
   *(.gnu.linkonce.t.*)
   *(.plt)
   *(.gnu_warning)
   *(.gcc_execpt_table)
   *(.glue_7)
   *(.glue_7t)
   *(.vfp11_veneer)
   *(.ARM.extab)
   *(.gnu.linkonce.armextab.*)
} > cpu0_ddr

.init : {
   KEEP (*(.init))
} > cpu0_ddr

.fini : {
   KEEP (*(.fini))
} > cpu0_ddr

.rodata : {
   __rodata_start = .;
   *(.rodata)
   *(.rodata.*)
   *(.gnu.linkonce.r.*)
   __rodata_end = .;
} > cpu0_ddr

.rodata1 : {
   __rodata1_start = .;
   *(.rodata1)
   *(.rodata1.*)
   __rodata1_end = .;
} > cpu0_ddr

.sdata2 : {
   __sdata2_start = .;
   *(.sdata2)
   *(.sdata2.*)
   *(.gnu.linkonce.s2.*)
   __sdata2_end = .;
} > cpu0_ddr

.sbss2 : {
   __sbss2_start = .;
   *(.sbss2)
   *(.sbss2.*)
   *(.gnu.linkonce.sb2.*)
   __sbss2_end = .;
} > cpu0_ddr

.data : {
   __data_start = .;
   *(.data)
   *(.data.*)
   *(.gnu.linkonce.d.*)
   *(.jcr)
   *(.got)
   *(.got.plt)
   __data_end = .;
} > cpu0_ddr

.data1 : {
   __data1_start = .;
   *(.data1)
   *(.data1.*)
   __data1_end = .;
} > cpu0_ddr

.got : {
   *(.got)
} > cpu0_ddr

.note.gnu.build-id : {
   KEEP (*(.note.gnu.build-id))
} > cpu0_ddr

.ctors : {
   __CTOR_LIST__ = .;
   ___CTORS_LIST___ = .;
   KEEP (*crtbegin.o(.ctors))
   KEEP (*(EXCLUDE_FILE(*crtend.o) .ctors))
   KEEP (*(SORT(.ctors.*)))
   KEEP (*(.ctors))
   __CTOR_END__ = .;
   ___CTORS_END___ = .;
} > cpu0_ddr

.dtors : {
   __DTOR_LIST__ = .;
   ___DTORS_LIST___ = .;
   KEEP (*crtbegin.o(.dtors))
   KEEP (*(EXCLUDE_FILE(*crtend.o) .dtors))
   KEEP (*(SORT(.dtors.*)))
   KEEP (*(.dtors))
   __DTOR_END__ = .;
   ___DTORS_END___ = .;
} > cpu0_ddr

.fixup : {
   __fixup_start = .;
   *(.fixup)
   __fixup_end = .;
} > cpu0_ddr

.eh_frame : {
   *(.eh_frame)
} > cpu0_ddr

.eh_framehdr : {
   __eh_framehdr_start = .;
   *(.eh_framehdr)
   __eh_framehdr_end = .;
} > cpu0_ddr

.gcc_except_table : {
   *(.gcc_except_table)
} > cpu0_ddr

.mmu_tbl (ALIGN(16384)) : {
   __mmu_tbl_start = .;
   *(.mmu_tbl)
   __mmu_tbl_end = .;
} > cpu0_ddr

.ARM.exidx : {
   __exidx_start = .;
   *(.ARM.exidx*)
   *(.gnu.linkonce.armexidix.*.*)
   __exidx_end = .;
} > cpu0_ddr

.preinit_array : {
   __preinit_array_start = .;
   KEEP (*(SORT(.preinit_array.*)))
   KEEP (*(.preinit_array))
   __preinit_array_end = .;
} > cpu0_ddr

.init_array : {
   __init_array_start = .;
   KEEP (*(SORT(.init_array.*)))
   KEEP (*(.init_array))
   __init_array_end = .;
} > cpu0_ddr

.fini_array : {
   __fini_array_start = .;
   KEEP (*(SORT(.fini_array.*)))
   KEEP (*(.fini_array))
   __fini_array_end = .;
} > cpu0_ddr

.ARM.attributes : {
   __ARM.attributes_start = .;
   *(.ARM.attributes)
   __ARM.attributes_end = .;
} > cpu0_ddr

.sdata : {
   __sdata_start = .;
   *(.sdata)
   *(.sdata.*)
   *(.gnu.linkonce.s.*)
   __sdata_end = .;
} > cpu0_ddr

.sbss (NOLOAD) : {
   __sbss_start = .;
   *(.sbss)
   *(.sbss.*)
   *(.gnu.linkonce.sb.*)
   __sbss_end = .;
} > cpu0_ddr

.tdata : {
   __tdata_start = .;
   *(.tdata)
   *(.tdata.*)
   *(.gnu.linkonce.td.*)
   __tdata_end = .;
} > cpu0_ddr

.tbss : {
   __tbss_start = .;
   *(.tbss)
   *(.tbss.*)
   *(.gnu.linkonce.tb.*)
   __tbss_end = .;
} > cpu0_ddr

.bss (NOLOAD) : {
   __bss_start = .;
   *(.bss)
   *(.bss.*)
   *(.gnu.linkonce.b.*)
   *(COMMON)
   __bss_end = .;
} > cpu0_ddr

_SDA_BASE_ = __sdata_start + ((__sbss_end - __sdata_start) / 2 );

_SDA2_BASE_ = __sdata2_start + ((__sbss2_end - __sdata2_start) / 2 );

/* Generate Stack and Heap definitions */

.heap (NOLOAD) : {
   . = ALIGN(16);
   _heap = .;
   HeapBase = .;
   _heap_start = .;
   . += _HEAP_SIZE;
   _heap_end = .;
   HeapLimit = .;
} > cpu0_ddr

.stack (NOLOAD) : {
   . = ALIGN(16);
   _stack_end = .;
   . += _STACK_SIZE;
   . = ALIGN(16);
   _stack = .;
   __stack = _stack;
   . = ALIGN(16);
   _irq_stack_end = .;
   . += _IRQ_STACK_SIZE;
   . = ALIGN(16);
   __irq_stack = .;
   _supervisor_stack_end = .;
   . += _SUPERVISOR_STACK_SIZE;
   . = ALIGN(16);
   __supervisor_stack = .;
   _abort_stack_end = .;
   . += _ABORT_STACK_SIZE;
   . = ALIGN(16);
   __abort_stack = .;
   _fiq_stack_end = .;
   . += _FIQ_STACK_SIZE;
   . = ALIGN(16);
   __fiq_stack = .;
   _undef_stack_end = .;
   . += _UNDEF_STACK_SIZE;
   . = ALIGN(16);
   __undef_stack = .;
} > cpu0_ddr

.dma1 (NOLOAD) : {
	. = ALIGN(16);
	_dma1_ddr_start = .;
	KEEP (*(.dma1))
	. = ALIGN(16);
	_dma1_ddr_end = .;
} > dma1_ddr

.dma2 (NOLOAD) : {
	. = ALIGN(16);
	_dma2_ddr_start = .;
	KEEP (*(.dma2))
	. = ALIGN(16);
	_dma2_ddr_end = .;
} > dma2_ddr

.dma3 (NOLOAD) : {
	. = ALIGN(16);
	_dma3_ddr_start = .;
	KEEP (*(.dma3))
	. = ALIGN(16);
	_dma3_ddr_end = .;
} > dma3_ddr

.dma4 (NOLOAD) : {
	. = ALIGN(16);
	_dma4_ddr_start = .;
	KEEP (*(.dma4))
	. = ALIGN(16);
	_dma4_ddr_end = .;
} > dma4_ddr

.queue1 (NOLOAD) : {
	. = ALIGN(16);
	_queue1_ddr_start = .;
	KEEP (*(.queue1))
	. = ALIGN(16);
	_queue1_ddr_end = .;
} > queue1_ddr

.queue2 (NOLOAD) : {
	. = ALIGN(16);
	_queue2_ddr_start = .;
	KEEP (*(.queue2))
	. = ALIGN(16);
	_queue2_ddr_end = .;
} > queue2_ddr

.queue3 (NOLOAD) : {
	. = ALIGN(16);
	_queue3_ddr_start = .;
	KEEP (*(.queue3))
	. = ALIGN(16);
	_queue3_ddr_end = .;
} > queue3_ddr

.queue4 (NOLOAD) : {
	. = ALIGN(16);
	_queue4_ddr_start = .;
	KEEP (*(.queue4))
	. = ALIGN(16);
	_queue4_ddr_end = .;
} > queue4_ddr

.unused (NOLOAD) : {
	. = ALIGN(16);
	_unused_ddr_start = .;
	KEEP (*(.unused))
	. = ALIGN(16);
	_unused_ddr_end = .;
} > unused_ddr

.vdisk (NOLOAD) : {
	. = ALIGN(16);
	_vdisk_ddr_start = .;
	KEEP (*(.vdisk))
	. = ALIGN(16);
	_vdisk_ddr_end = .;
} > vdisk_ddr

_end = .;
}

