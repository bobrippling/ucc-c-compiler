// http://en.wikipedia.org/wiki/One_Definition_Rule

struct S;   // declaration of S
struct S * p;      // ok, no definition required
void f(struct S*); // ok, no definition required
//void g(struct S);  // ok, no definition required
struct S *h();      // ok, no definition required

main()
{
	//struct S s;        // error, definition required
	//sizeof(struct S);  // error, definition required
}
