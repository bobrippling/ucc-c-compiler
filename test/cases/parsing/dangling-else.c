// RUN: %check %s

int g(void);

void f(int cond, int cond2)
{
	if(cond)
		if(cond2)
			g();
		else // CHECK: warning: dangling else statement
			g();

	if(cond)
		if(cond2){
			g();
		}else // CHECK: warning: dangling else statement
			g();

	if(cond)
		if(cond2){
			g();
		}else{ // CHECK: warning: dangling else statement
			g();
		}

	if(cond){
		if(cond2)
			g();
		else // CHECK: !/warn.*dangling/
			g();
	}

	if(cond)
		for(;;)
			if(cond2){
				g();
			}
	else // CHECK: warning: dangling else statement
		g();


	if(cond)
		for(;;)
			if(cond2)
				g();
	else // CHECK: warning: dangling else statement
		g();

	if(cond)
		({
			if(cond2)
				g();
			else // CHECK: !/warn.*dangling/
				g();
		});

		if(cond)
			g();
		else if(cond2) // CHECK: !/warn.*dangling/
			g();
		else if(cond ^ cond2) // CHECK: !/warn.*dangling/
			g();
		else if(cond | cond2) // CHECK: !/warn.*dangling/
			g();
}
