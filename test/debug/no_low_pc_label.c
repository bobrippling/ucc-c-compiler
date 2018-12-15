// RUN: %ucc -g %s -o %t

main()
{
	if(1){
		__builtin_trap();
	}else{
	}
}
