/* include/ui/ui_menu_actions.h
 *
 * Action selection menu.
 */

#ifndef UI_MENU_ACTIONS_H
#define UI_MENU_ACTIONS_H

#include <editor/syntax.h>
#include <ui/ui_avy.h>
#include <ui/ui_types.h>

void menu_actions_draw(struct ui_ctx *ctx,
		       ui_rect rect,
		       struct avy_match *match,
		       struct str line_text,
		       const struct syntax_visible *ast);

#endif /* UI_MENU_ACTIONS_H */
