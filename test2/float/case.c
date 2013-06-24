// RUN: %check -e %s

main()
{
	switch(0){
		case 1.0: // CHECK: /error: float in case/
			;
	}
}
