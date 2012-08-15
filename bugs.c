f(a)
{
	x() || y(); // checks rbx instead of rax for func ret
}

g(a)
{
	switch(a){
		case 5: // out_jeq causes unnecessary reload from stack
		case 2:
			;
	}
}
