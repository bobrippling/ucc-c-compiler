// RUN: %check -e %s
f(a)
{
	switch(a){
		case 1 ... 1:; // CHECK: error: case range equal or inverse
	}
}
