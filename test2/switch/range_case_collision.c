// RUN: %check -e %s
f(int a)
{
	switch(a){
		case 2:
			break;
		case 1 ... 3: // CHECK: error: overlapping case statements starting at 2
			g();
	}
}
