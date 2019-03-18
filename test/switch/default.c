// RUN: %check %s -Wswitch-default

main()
{
	switch(0){ // CHECK: warning: switch has no default label
	}
}
