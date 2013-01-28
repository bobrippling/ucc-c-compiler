// RUN: %ucc -DAFTER=         -DBEFORE='int a;' %s; [ $? -ne 0 ]
// RUN: %ucc -DAFTER='int a;' -DBEFORE=         %s; [ $? -ne 0 ]

struct
{
	BEFORE
	struct
	{
		int b, a;
	};
	AFTER
};
