// RUN: %check %s

typedef struct {
	int x, y;
} point;

typedef struct {
	point a, b;
} lineseg;

lineseg seg;

point pointmake(int x, int y)
{
	return (point){
		.x = x,
		.y = y,
	};
}

main()
{
	seg = (lineseg){ .b = pointmake(1, 2), .a = pointmake(5, 6) }; // CHECK: !/unused/
}
