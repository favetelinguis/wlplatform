/* include/ui/ui_menu_ast.h
 *
 * AST display menu.
 */

#ifndef UI_MENU_AST_H
#define UI_MENU_AST_H

#include <editor/syntax.h>
#include <ui/ui_types.h>

void menu_ast_draw(struct ui_ctx *ctx,
		   ui_rect rect,
		   const struct syntax_visible *visible,
		   int cursor_row);

#endif /* UI_MENU_AST_H */
