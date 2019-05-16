// RUN: %ocheck 6 %s

main()
{
	^(){
	}();

	return ^(int a){
		return a + 1;
	}(5);
}
