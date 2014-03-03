// RUN: %check -e %s
f()
{
	switch(5){
		default:; // CHECK: note: other default here
		case 3:;
		default:; // CHECK: error: duplicate default statement
	}
}
