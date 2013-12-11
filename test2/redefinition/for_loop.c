// RUN: %check %s -Wshadow

main()
{
	for(int i = 0; i < 10; i++){ // CHECK: note: local declaration here
		int i = 2; // CHECK: warning: declaration of "i" shadows local declaration
	}
}
