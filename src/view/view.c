#include "view.h"

void
view_init(struct view *v)
{
	v->first_visible_line = -1;
	v->last_visible_line = -1;
	v->cursor_line = 0;
	v->lines_above = 0;
	v->lines_below = 0;
	v->needs_ast_update = true;
}

bool
view_update(struct view *v,
	    int cursor_line,
	    int line_count,
	    int window_h,
	    int line_h,
	    int menu_h)
{
	int input_h = line_h + 4;
	int input_y = (window_h - input_h) / 2;
	int lines_above = input_y / line_h;
	int lines_below = (window_h - input_y - input_h - menu_h) / line_h;

	int first = cursor_line - lines_above;
	if (first < 0)
		first = 0;

	int last = cursor_line + lines_below;
	if (last >= line_count)
		last = line_count - 1;

	bool changed =
	    (first != v->first_visible_line || last != v->last_visible_line);

	v->cursor_line = cursor_line;
	v->lines_above = lines_above;
	v->lines_below = lines_below;
	v->first_visible_line = first;
	v->last_visible_line = last;
	v->needs_ast_update = changed;

	return changed;
}
