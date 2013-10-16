.section .text

.Ldbg_lowpc:

.globl _start
.Ldbg_begin_start:
_start:
	movl $60, %eax
	movl $32, %edi
	syscall
	hlt
.Ldbg_end_start:

.Ldbg_highpc:

.section .data
/* int i = 7 */
.globl i
i:
	.long 7


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

	.byte 2 // abbrev. code
		.byte DW_TAG_subprogram, DW_CHILDREN_no
			.byte DW_AT_name, DW_FORM_string
			.byte DW_AT_decl_file, DW_FORM_data1
			.byte DW_AT_decl_line, DW_FORM_data1
			.byte DW_AT_type, DW_FORM_ref4
			.byte DW_AT_external, DW_FORM_flag
			.byte DW_AT_low_pc, DW_FORM_addr
			.byte DW_AT_high_pc, DW_FORM_addr
		.byte 0
	.byte 0


	.byte 3 // abrv for int
		.byte DW_TAG_base_type, DW_CHILDREN_no
			.byte DW_AT_byte_size, DW_FORM_data1
			.byte DW_AT_encoding, DW_FORM_data1
			.byte DW_AT_name, DW_FORM_string
		.byte 0
	.byte 0
	.byte 4 // abrv for long
		.byte DW_TAG_base_type, DW_CHILDREN_no
			.byte DW_AT_byte_size, DW_FORM_data1
			.byte DW_AT_encoding, DW_FORM_data1
			.byte DW_AT_name, DW_FORM_string
		.byte 0
	.byte 0
	.byte 5 // abrv for double
		.byte DW_TAG_base_type, DW_CHILDREN_no
			.byte DW_AT_byte_size, DW_FORM_data1
			.byte DW_AT_encoding, DW_FORM_data1
			.byte DW_AT_name, DW_FORM_string
		.byte 0
	.byte 0

	.byte 6 // abrv for `int i'
		.byte DW_TAG_variable, DW_CHILDREN_no
			.byte DW_AT_name, DW_FORM_string
			.byte DW_AT_type, DW_FORM_ref4
		.byte 0
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

	.byte 2 // DW_TAG_subprogram reference
		.asciz "start_fn"
		.byte 1 // TODO: refer to decl_file / yo_synthetic.c
		.byte 5 // line #5
		.long 0 // TODO: DW_AT_type
		.byte 1
		.quad .Ldbg_begin_start
		.quad .Ldbg_end_start

.Ldbg_type_int:
	.byte 3 // DW_TAG_base_type reference
		.byte 4 // sizeof(int)
		.byte DW_ATE_signed
		.asciz "int"
	.byte 4 // DW_TAG_base_type reference
		.byte 8
		.byte DW_ATE_signed
		.asciz "long"
	.byte 5 // DW_TAG_base_type reference
		.byte 8 // sizeof(int)
		.byte DW_ATE_float
		.asciz "double"

	.byte 6
		.asciz "i"
		.quad .Ldbg_type_int

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
