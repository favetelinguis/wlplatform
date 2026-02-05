#include "ui_menu_actions.h"

#include <stdio.h>
#include <string.h>

#include "../render/render_font.h"
#include "../render/render_primitives.h"
#include "../string/str.h"
#include "ui_label.h"

/* Zenburn blue for action highlights */
#define ZENBURN_BLUE 0xFF8CD0D3

void
menu_actions_draw(struct ui_ctx *ctx,
		  struct ui_rect rect,
		  struct avy_match *match,
		  str line_text,
		  const struct syntax_visible *ast)
{
	int line_h;
	int y, x;
	char buf[256];
	int i;
	const struct syntax_node *node;
	const struct syntax_node *containing;

	line_h = font_get_line_height(ctx->font);
	y = rect.y;
	x = 8;
	containing = NULL;

	/* Background */
	draw_rect(ctx, rect, ctx->theme.bg_secondary);

	/* Header */
	ui_label_draw_colored(
	    ctx, x, y, STR_LIT("Actions:"), ctx->theme.accent);
	y += line_h;

	/* Show target info */
	snprintf(buf,
		 sizeof(buf),
		 "Target: line %d, col %d",
		 match->line + 1,
		 match->col);
	ui_label_draw_colored(
	    ctx, x, y, str_from_cstr(buf), ctx->theme.fg_secondary);
	y += line_h;

	/* Show line preview (truncated) */
	{
		int max_preview = 60;
		int len = (int)str_len(line_text);
		if (len > max_preview) {
			snprintf(buf,
				 sizeof(buf),
				 "  \"%.*s...\"",
				 max_preview,
				 str_data(line_text));
		} else {
			snprintf(buf,
				 sizeof(buf),
				 "  \"%.*s\"",
				 len,
				 str_data(line_text));
		}
		ui_label_draw_colored(
		    ctx, x, y, str_from_cstr(buf), ctx->theme.fg_muted);
		y += line_h;
	}

	/* Find containing AST node (deepest node containing the match line) */
	for (i = 0; i < ast->count; i++) {
		node = &ast->nodes[i];
		if ((int)node->start_row <= match->line &&
		    (int)node->end_row >= match->line) {
			/* Prefer deeper nodes (more specific context) */
			if (containing == NULL ||
			    node->depth > containing->depth) {
				containing = node;
			}
		}
	}

	/* Show AST context */
	y += line_h / 2; /* Small visual gap */
	ui_label_draw_colored(
	    ctx, x, y, STR_LIT("AST Context:"), ctx->theme.accent);
	y += line_h;

	if (containing) {
		snprintf(buf, sizeof(buf), "  Node: %s", containing->type);
		ui_label_draw_colored(
		    ctx, x, y, str_from_cstr(buf), ctx->theme.fg_primary);
		y += line_h;

		snprintf(buf,
			 sizeof(buf),
			 "  Range: [%u:%u] - [%u:%u]",
			 containing->start_row,
			 containing->start_col,
			 containing->end_row,
			 containing->end_col);
		ui_label_draw_colored(
		    ctx, x, y, str_from_cstr(buf), ctx->theme.fg_secondary);
		y += line_h;
	} else {
		ui_label_draw_colored(
		    ctx, x, y, STR_LIT("  (no node)"), ctx->theme.fg_muted);
		y += line_h;
	}

	/* Available actions section */
	y += line_h / 2;
	ui_label_draw_colored(ctx, x, y, STR_LIT("Press:"), ctx->theme.accent);
	y += line_h;

	/* Jump action */
	ui_label_draw_colored(
	    ctx, x, y, STR_LIT("  [j] Jump to line"), ZENBURN_BLUE);
	y += line_h;

	/* Cancel hint */
	ui_label_draw_colored(
	    ctx, x, y, STR_LIT("  [Esc] Cancel"), ctx->theme.fg_muted);
}
