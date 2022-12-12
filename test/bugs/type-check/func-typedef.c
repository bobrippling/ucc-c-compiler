#if BUG_1
void*const (*y)(size_t);
//              ^~~~~~ invalid in non-def

#elif BUG_2
void*const (*y)(size_t);

f(){
	*y=0; // should be ok, ucc error on assignment
}
#else

typedef int size_t;
void*const (*y)(size_t);
void*const g(size_t);
f(){
	y = g; // should be ok, ucc error on assignment
}
#endif
