#ifndef TREE_H
#define TREE_H

typedef struct
{
	enum stat_type
	{
		stat_do,
		stat_if,
		stat_else,
		stat_while,
		stat_for,
		stat_break,
		stat_return,

typedef struct
{
	char *spel;
	list *statements;
} function;

#endif
