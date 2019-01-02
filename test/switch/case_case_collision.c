// RUN: %check -e %s
f(int i)
{
	switch(i){
		case 1:
		case 2:
		case 1: // CHECK: error: duplicate case statements for 1
			break;
	}
}
