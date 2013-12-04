// RUN: %check %s
enum
{
	A, B, C = B
};

main()
{
	switch((typeof(A))0){ // CHECK: /warning: enum ::A not handled in switch/
		case 5: // CHECK: /warning: 'case 5' not a member of enum/
		case B:
			;
	}
}
