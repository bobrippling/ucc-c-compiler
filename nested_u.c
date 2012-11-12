#ifdef COMPLEX
struct format_option {
	union {
		const char * fo_str ;
		int fo_int ;
		int fo_time ;
	} ;
	unsigned int empty : 1 ;
	enum { FO_STR , FO_INT , FO_TIME } type ;
	char ch ;
} ;

static struct format_option track_fopts [ 11 ] = {
	{ { . fo_str = ( (void *)0 ) } , 0 , FO_STR , 'a' } ,
	{ { . fo_str = ( (void *)0 ) } , 0 , FO_STR , 'l' } ,
	{ { . fo_int = 0             } , 0 , FO_INT , 'D' } ,
	{ { . fo_int = 0             } , 0 , FO_INT , 'n' } ,
	{ { . fo_str = ( (void *)0 ) } , 0 , FO_STR , 't' } ,
	{ { . fo_str = ( (void *)0 ) } , 0 , FO_STR , 'y' } ,
	{ { . fo_str = ( (void *)0 ) } , 0 , FO_STR , 'g' } ,
	{ { . fo_time = 0            } , 0 , FO_TIME, 'd' } ,
	{ { . fo_str = ( (void *)0 ) } , 0 , FO_STR , 'f' } ,
	{ { . fo_str = ( (void *)0 ) } , 0 , FO_STR , 'F' } ,

	{ { . fo_str = ( (void *)0 ) } , 0 , 0 , 0 }
} ;
#else
struct foo
{
    union{
        int i;
    };
};

static struct foo f = {
    { .i = 0 }
};
#endif
