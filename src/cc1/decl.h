#ifndef DECL_H
#define DECL_H

decl        *decl_new(void);
array_decl  *array_decl_new(void);
decl_attr   *decl_attr_new(enum decl_attr_type);

decl_desc   *decl_desc_new(enum decl_desc_type t);
decl_desc   *decl_desc_spel_new(char *sp);
decl_desc   *decl_desc_ptr_new(void);
decl_desc   *decl_desc_func_new(void);
decl_desc   *decl_desc_array_new(void);

decl      *decl_copy(decl *);
decl_desc *decl_desc_copy(decl_desc *);

int   decl_size( decl *);
int   decl_equal(decl *, decl *, enum decl_cmp mode);

int     decl_is_struct_or_union(decl *);
int     decl_is_callable(       decl *);
int     decl_is_func(           decl *); /* different from _callable - fptrs are also callable */
int     decl_is_const(          decl *);
int     decl_ptr_depth(         decl *);
#define decl_is_void(d) ((d)->type->primitive == type_void && !(d)->desc)

decl_desc  *decl_first_func(decl *d);
decl_desc  *decl_leaf(decl *d);

decl *decl_desc_depth_inc(decl *d);
decl *decl_desc_depth_dec(decl *d);
void  decl_func_deref(decl *d, funcargs **pfuncargs);

int decl_attr_present(decl_attr *, enum decl_attr_type);

int decl_has_array(decl *);
funcargs *decl_funcargs(void);

const char *decl_desc_str(decl_desc *dp);

char *decl_spel(decl *);
void  decl_set_spel(decl *, char *);

void decl_desc_free(decl_desc *);
#define decl_free_notype(x) do{free(x);}while(0)
#define decl_free(x) do{type_free((x)->type); decl_free_notype(x);}while(0)

#endif
