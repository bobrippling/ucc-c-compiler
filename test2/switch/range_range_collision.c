// RUN: %check -e %s
f(a)
{
	switch(a){
		case 1 ... 3: // CHECK: error: overlapping case statements starting at 3
		case 3 ... 5:
			break;
	}
}
