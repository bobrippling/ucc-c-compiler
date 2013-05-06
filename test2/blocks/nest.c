// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 5 ]

main()
{
	return ^(int i){
		int (^b)(int);

		b = ^(int a){
			return ^(int f){
				return ({
						int a = f;
						a;
					});
			}(a);
		};

		return b(i);
	}(5);
}
