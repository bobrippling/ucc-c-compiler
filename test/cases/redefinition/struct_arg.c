// RUN: %check -e %s

typedef struct point point_t;
typedef _Bool bool;

void ui_draw_completion( // CHECK: note: previous definition
		struct hash *ents, const int sel, // CHECK: warning: declaration of 'struct hash' only visible inside function
		point_t const *at,
		bool is_hidden(const void *), char *get_str(const void *));

void ui_draw_completion( // CHECK: error: mismatching definitions of "ui_draw_completion"
		struct hash *ents, const int sel, // CHECK: warning: declaration of 'struct hash' only visible inside function
		point_t const *at,
		bool is_hidden(const void *), char *get_str(const void *));
