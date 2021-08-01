// RUN: %check -e %s

main()
{ 
	int x = 2;  

	switch(x){  
		case 1,2: // CHECK: error: expecting token ':', got ','
			break;  
	}  
}
