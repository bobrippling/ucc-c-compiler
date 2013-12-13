// RUN: %check -e %s
f(a)
{
	switch(a){
		case 5 ... 3:; // CHECK: error: case range equal or inverse
	}
}
