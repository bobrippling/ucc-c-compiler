// RUN: %check --only %s -Wno-dead-code -Wno-int-ptr-conversion

main()
{
	^{
		return;
		return 3; // CHECK: /warning: return with a value in/
		return (char *)3; // CHECK: /warning: return with a value in/
	}();

	^{
		return 3;
		return; // CHECK: /warning: empty return/
		return (char *)3; // CHECK: /warning: mismatching types, return type/
	}();
}
