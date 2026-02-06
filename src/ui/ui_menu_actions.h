#ifndef UI_MENU_ACTIONS_H
#define UI_MENU_ACTIONS_H

#include "../syntax/syntax.h"
#include "ui_avy.h"
#include "ui_types.h"

/*
 * Draw the action menu in the menu area.
 * Shows available actions and the AST node context for the selected match.
 */
void menu_actions_draw(struct ui_ctx *ctx,
		       struct ui_rect rect,
		       struct avy_match *match,
		       struct str line_text,
		       const struct syntax_visible *ast);

#endif /* UI_MENU_ACTIONS_H */
