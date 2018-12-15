// RUN: %check -e %s

f()
{
	switch(q()){
		case g(): // CHECK: /error: integral constant expected/
			;
	}
}
