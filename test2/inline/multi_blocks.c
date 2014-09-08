// RUN: %check %s -fshow-inlined

main()
{
	return ^(void (*fn)(int *)){ // CHECK: note: function inlined
		int i;
		fn(&i);
		return i;
	}(^{ // CHECK: note: function inlined
			return ^(int *p){
				*p = 7;
			};
		}());
}
