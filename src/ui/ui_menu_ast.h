#ifndef MENU_AST_H
#define MENU_AST_H

#include "../syntax/syntax.h"
#include "../ui/ui_types.h"

void menu_ast_draw(struct ui_ctx *ctx,
		   struct ui_rect rect,
		   const struct syntax_visible *visible,
		   int cursor_row);

#endif
