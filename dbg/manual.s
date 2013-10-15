.section .text

.Ldbg_lowpc:

.globl _start
_start:
	movl $60, %eax
	movl $32, %edi
	syscall
	hlt

.Ldbg_highpc:


// debugging sections

#include <dwarf.h>

.section .debug_abbrev
	.byte 1 // abbrev. code
		.byte DW_TAG_compile_unit, DW_CHILDREN_yes
			.byte DW_AT_producer, DW_FORM_string
			.byte DW_AT_language, DW_FORM_data2
			.byte DW_AT_name, DW_FORM_string

			.byte DW_AT_low_pc, DW_FORM_addr
			.byte DW_AT_high_pc, DW_FORM_addr

			.byte DW_AT_comp_dir, DW_FORM_string
		.byte 0 // terminate
	.byte 0




.section .debug_info

	.long .Ldbg_info_end - .Ldbg_info_start // length
.Ldbg_info_start:
	.short 2 // DWARF version 2
	.long 0 // offset into abbrev section
	.byte 8 // sizeof(void *)

	.byte 1 // DW_TAG_compile_unit reference
		.asciz "ucc development version"
		.short DW_LANG_C99 // data2 XXX: or C89
		.asciz "yo_synthetic.c"
		.quad .Ldbg_lowpc
		.quad .Ldbg_highpc
		.asciz "."

.Ldbg_info_end:


#if 0
.debug_aranges   // map address => compilation unit
.debug_frame     // call frame information
.debug_line      // line number information
.debug_loc       // location lists used in the DW_AT_location attributes
.debug_macinfo   // macro information
.debug_pubnames  // lookup table for global objects and functions
.debug_pubtypes  // lookup table for global types
.debug_ranges    // address ranges used in the DW_AT_ranges attributes
.debug_str       // string table used in .debug_info
.debug_types     // type descriptions
#endif
