// RUN: %ocheck 5 %s

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
