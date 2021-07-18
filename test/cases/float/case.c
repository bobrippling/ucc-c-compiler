// RUN: %check -e %s

main()
{
	switch(0){
		case 1.0: // CHECK: error: case requires an integral type
			;
	}
}
