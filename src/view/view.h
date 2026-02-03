#ifndef VIEW_H
#define VIEW_H

#include <stdbool.h>

struct view {
	int first_visible_line;
	int last_visible_line;
	int cursor_line;
	int lines_above;
	int lines_below;
	bool needs_ast_update;
};

void view_init(struct view *v);

/* Returns true if visible range changed */
bool view_update(struct view *v,
		 int cursor_line,
		 int line_count,
		 int window_h,
		 int line_h,
		 int menu_h);

#endif
