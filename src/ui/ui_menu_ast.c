#include <ui/ui_menu_ast.h>

#include <stdio.h>
#include <string.h>

#include <render/render_primitives.h>
#include <ui/ui_label.h>
#include <ui/ui_panel.h>

#define INDENT_SPACES	 2
#define MAX_LINE	 128
#define MAX_TEXT_PREVIEW 24

void
menu_ast_draw(struct ui_ctx *ctx,
	      ui_rect rect,
	      const struct syntax_visible *visible,
	      int cursor_row)
{
	int line_h = ui_label_height(ctx);
	int padding = 8;
	int max_lines = (rect.h - padding * 2) / line_h;
	int y = rect.y + padding;
	char line[MAX_LINE];
	char text_preview[MAX_TEXT_PREVIEW + 4]; /* +4 for "..." and NUL */

	/* Background */
	ui_panel_draw(ctx, rect, ctx->theme.bg_hover, UI_PANEL_FLAT);

	if (max_lines <= 0)
		return;

	/* Header */
	ui_label_draw_colored(ctx,
			      rect.x + padding,
			      y,
			      STR_LIT("AST (visible):"),
			      ctx->theme.accent);
	y += line_h;
	max_lines--;

	/* Nodes */
	for (int i = 0; i < visible->count && i < max_lines; i++) {
		const struct syntax_node *n = &visible->nodes[i];
		int indent = n->depth * INDENT_SPACES;
		if (indent > 16)
			indent = 16;

		/* Prepare text preview for leaf nodes */
		if (!str_empty(n->text)) {
			int preview_len = str_len(n->text);
			if (preview_len > MAX_TEXT_PREVIEW)
				preview_len = MAX_TEXT_PREVIEW;

			memcpy(text_preview, str_data(n->text),
			       (size_t)preview_len);
			/* Replace newlines with visible marker */
			for (int j = 0; j < preview_len; j++) {
				if (text_preview[j] == '\n')
					text_preview[j] = ' ';
			}
			if (str_len(n->text) > MAX_TEXT_PREVIEW) {
				memcpy(text_preview + preview_len, "...", 4);
			} else {
				text_preview[preview_len] = '\0';
			}

			snprintf(line,
				 MAX_LINE,
				 "%*s%s [%u:%u] \"%s\"",
				 indent,
				 "",
				 n->type,
				 n->start_row,
				 n->start_col,
				 text_preview);
		} else {
			snprintf(line,
				 MAX_LINE,
				 "%*s%s [%u:%u-%u:%u]",
				 indent,
				 "",
				 n->type,
				 n->start_row,
				 n->start_col,
				 n->end_row,
				 n->end_col);
		}

		/* Highlight if cursor is within this node */
		uint32_t color = ctx->theme.fg_secondary;
		if ((uint32_t)cursor_row >= n->start_row &&
		    (uint32_t)cursor_row <= n->end_row) {
			color = ctx->theme.fg_primary;
		}

		ui_label_draw_colored(
		    ctx, rect.x + padding, y, str_from_cstr(line), color);
		y += line_h;
	}

	/* Truncation indicator */
	if (visible->count > max_lines) {
		snprintf(line,
			 MAX_LINE,
			 "... +%d more",
			 visible->count - max_lines);
		ui_label_draw_colored(ctx,
				      rect.x + padding,
				      y,
				      str_from_cstr(line),
				      ctx->theme.fg_muted);
	}
}
