// RUN: %check -e %s
f(a)
{
	switch(a){
		case 1 ... 10: // CHECK: error: overlapping case statements starting at 9
		case 9 ... 13:
			break;

		default:
			;
	}
}
