// RUN: [ `%ucc %s -S -o- | grep 'mov.*[1234]' | wc -l` -eq 4 ]
// RUN: %ucc %s -S -o- | grep 5; [ $? -ne 0 ]

main()
{
	int (*p)[] = (int[][2]){ 1, 2, { 3, 4, 5 } };
}
