#ifndef ENUM_H
#define ENUM_H

typedef struct enum_member
{
	char *spel;
	expr *val; /* (expr *)-1 if not given */
	int tag; /* for switch() checking */
} enum_member;

struct enum_st
{
	char *spel; /* NULL if anon */
	enum_member **members;
};

void enum_vals_add(enum_st *, char *, expr *);

enum_st *enum_add(enum_st ***ens, char *spel, enum_st *en);
enum_st     *enum_find(       symtable *, const char *spel);
enum_member *enum_find_member(symtable *, const char *spel);

int      enum_nentries(enum_st *);
enum_st *enum_st_new(void);

#endif
