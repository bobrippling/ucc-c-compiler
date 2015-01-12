#include "op.h"

int op_exe(enum op op, int l, int r)
{
	switch(op){
		case op_add:
			return l + r;
	}
}
