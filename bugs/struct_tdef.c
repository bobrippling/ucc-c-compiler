main()
{
	typedef struct
	{
		int x, y;
	} ui_pos;

	ui_pos p;

	((ui_pos *)0)->x;
	p.x = 5;
}
