main()
{
	enum op
	{
		OP_NUM_1,
		OP_NUM_2,
		OP_NUM_3,
		OP_NUM_4,
		OP_ADD,
		OP_PRINT,
		OP_HLT
	} cmds[] = {
		OP_NUM_1,
		OP_NUM_2,
		OP_ADD,
		OP_PRINT,
		OP_HLT
	},
	opp; opp = cmds;

	void *ops[] = {
		&&op_num1,
		&&op_num2,
		&&op_num3,
		&&op_num4,
		&&op_add,
		&&op_print,
		&&op_hlt
	};

	goto *opp++;


op_num1:
op_num2:
op_num3:
op_num4:
op_add:
op_print:

op_hlt:
	return 0;
}
