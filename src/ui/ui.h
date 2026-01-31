#ifndef UI_H
#define UI_H

/*
 * Unified UI component library.
 *
 * Usage:
 *   #include "ui/ui.h"
 *
 *   struct ui_ctx ctx;
 *   ui_ctx_init(&ctx, fb, font);
 *   ui_ctx_clear(&ctx);
 *
 *   ui_label_draw(&ctx, 50, 20, "Title", UI_LABEL_NORMAL);
 *   ui_button_draw(&ctx, rect, &btn_cfg);
 */

#include "ui_button.h"
#include "ui_ctx.h"
#include "ui_label.h"
#include "ui_panel.h"
#include "ui_types.h"

#endif /* UI_H */
