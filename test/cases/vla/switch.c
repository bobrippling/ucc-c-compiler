// RUN: %check -e %s

f(int i, int n)
{
	switch(i){
		case 0:
		{
			short vla[n]; // CHECK: note: variable "vla"
		case 1: // CHECK: error: case inside scope of variably modified declaration
			return sizeof vla;
		}
	}
	return 0;
}
