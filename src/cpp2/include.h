#ifndef INCLUDE_H
#define INCLUDE_H

void include_add_dir(char *, int sysh);

/* *final_path and *is_sysh are only defined on non-null return */
FILE *include_fopen(
		const char *curdir,
		const char *fname,
		int is_angle,
		char **final_path,
		int *is_sysh);

#endif
