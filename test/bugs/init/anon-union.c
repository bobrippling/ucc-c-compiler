/*
struct sigevent {
	union sigval sigev_value;
	int sigev_signo;
	int sigev_notify;
	//     void (*sigev_notify_function)(union sigval);
	//     pthread_attr_t *sigev_notify_attributes;
	//     char __pad[56-3*sizeof(long)];
	union {
		char __pad[64 - 2*sizeof(int) - sizeof(union sigval)];
		pid_t sigev_notify_thread_id;
		struct {
			void (*sigev_notify_function)(union sigval);
			pthread_attr_t *sigev_notify_attributes;
		} __sev_thread;
	} __sev_fields;
};

#define sigev_notify_thread_id __sev_fields.sigev_notify_thread_id
#define sigev_notify_function __sev_fields.__sev_thread.sigev_notify_function
#define sigev_notify_attributes __sev_fields.__sev_thread.sigev_notify_attributes
*/

#ifdef MANY
union U {
	struct {
		union {
			struct {
				int a;
				int b;
			};
			int c;
		};
		int d;
	};
	int e;
};

union U global = {
	.a = 1,
	.b = 2,
	.c = 3, // overrides .a = 1
	.d = 4,
	.e = 5, // overrides .c = 3
};

#ifdef WITH_CODE
printf();

static void show(union U *u)
{
	printf("%d %d %d %d %d\n",
		u->a,
		u->b,
		u->c,
		u->d,
		u->e);
}

int main()
{
	union U local = {
		.a = 1,
		.b = 2,
		.c = 3, // overrides .a = 1
		.d = 4,
		.e = 5, // overrides .c = 3
	};

	// should be: 5 0 5 0 5

	show(&local);
	show(&global);
}
#endif
#else
struct U {
	union {
		struct {
			int a;
			int b;
		};
		int c;
	};
	int d;
};

struct U global = {
	.a = 1,
	.b = 2,
	.c = 3, // overrides .a = 1
	.d = 4,
};
#endif
