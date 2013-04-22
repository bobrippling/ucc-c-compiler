t_if()
{
	// if can define _one_ variable
	if(int i = f(), i > 5)
		abort();
}

t_while()
{
	while(int i = f(), i > 5)
		abort();
}

t_switch()
{
	switch(int i = f(), i++)
		abort();
}

t_for()
{
	// for() can define multiple variables
	for(int i = 5, j = 2; i + j < 10; j++);
}
