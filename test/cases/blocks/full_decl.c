// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 6 ]
main()
{
	^(){
	}();

	return ^(int a){
		return a + 1;
	}(5);
}
