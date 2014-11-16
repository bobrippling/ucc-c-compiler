typedef struct opaque *id;

extern id (*_imp(id))();

typedef char *SEL;

id f(id self, SEL _cmd, id arg)
{
	id local1, local2;

	return (
			local1 = (id)self,
			_imp(local1)(
				local1,
				arg
				?
					local2 = arg,
					_imp(local2)(local2)
				: (id)0));
}
