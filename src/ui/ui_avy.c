#include <ui/ui_avy.h>

#include <ctype.h>
#include <string.h>

#include <render/render_font.h>
#include <render/render_primitives.h>
#include <ui/ui_label.h>

#define ZENBURN_BLUE                                                          \
	0xFF8CD0D3 /* TODO this should live with all other theme colors and   \
		      not here */

static const char hint_chars[] = AVY_HINT_CHARS;
static const int hint_chars_len = sizeof(hint_chars) - 1; /* Exclude null */

void
avy_init(struct avy_state *avy)
{
	memset(avy, 0, sizeof(*avy));
	avy->selected_match = -1;
}

void
avy_start(struct avy_state *avy, enum avy_direction dir)
{
	avy_init(avy);
	avy->active = true;
	avy->direction = dir;
}

void
avy_cancel(struct avy_state *avy)
{
	avy_init(avy);
}

/*
 * Generate hint strings from n matches.
 */
static void
generate_hints(struct avy_match *matches, int n)
{
	int i, j, idx;

	if (n <= hint_chars_len) {
		/* Single character hints */
		for (i = 0; i < n; i++) {
			matches[i].hint[0] = hint_chars[i];
			matches[i].hint[1] = '\0';
		}
	} else {
		/* Two character hints */
		idx = 0;
		for (i = 0; i < hint_chars_len && idx < n; i++) {
			for (j = 0; j < hint_chars_len && idx < n; j++) {
				matches[idx].hint[0] = hint_chars[i];
				matches[idx].hint[1] = hint_chars[j];
				matches[idx].hint[2] = '\0';
				idx++;
			}
		}
	}
}

/*
 * Check if position col is at the start of a word.
 */
static bool
is_word_start(const char *data, int len, int col)
{
	if (col < 0 || col >= len)
		return false;
	if (col == 0)
		return true;
	/* Previous char must not be a word character */
	return !isalnum((unsigned char)data[col - 1]) && data[col - 1] != '_';
}

void
avy_set_char(struct avy_state *avy,
	     char c,
	     const struct str *lines,
	     int line_count,
	     int cursor_line,
	     int first_visible,
	     int last_visible)
{
	int line_num, col;
	const char *data;
	int len;

	avy->search_char = c;
	avy->match_count = 0;

	if (avy->direction == AVY_DIR_UP) {
		/* Search lines above cursor (from closest to furthest) */
		for (line_num = cursor_line - 1;
		     line_num >= first_visible &&
		     avy->match_count < AVY_MAX_MATCHES;
		     line_num--) {
			if (line_num < 0 || line_num >= line_count)
				continue;
			data = str_data(lines[line_num]);
			len = str_len(lines[line_num]);

			for (col = 0; col < len; col++) {
				/* Match exact case at word starts only */
				if (data[col] == c &&
				    is_word_start(data, len, col)) {
					avy->matches[avy->match_count].line =
					    line_num;
					avy->matches[avy->match_count].col =
					    col;
					avy->match_count++;
					if (avy->match_count >=
					    AVY_MAX_MATCHES)
						break;
				}
			}
		}
	} else {
		/* Search lines below cursor (from closest to furthest) */
		for (line_num = cursor_line + 1;
		     line_num <= last_visible &&
		     avy->match_count < AVY_MAX_MATCHES;
		     line_num++) {
			if (line_num < 0 || line_num >= line_count)
				continue;
			data = str_data(lines[line_num]);
			len = str_len(lines[line_num]);

			for (col = 0; col < len; col++) {
				/* Match exact case at word starts only */
				if (data[col] == c &&
				    is_word_start(data, len, col)) {
					avy->matches[avy->match_count].line =
					    line_num;
					avy->matches[avy->match_count].col =
					    col;
					avy->match_count++;
					if (avy->match_count >=
					    AVY_MAX_MATCHES)
						break;
				}
			}
		}
	}

	/* Generate hint labels for all matches */
	generate_hints(avy->matches, avy->match_count);
}

bool
avy_input_hint(struct avy_state *avy, char c)
{
	int i;
	int candidates;
	int last_candidate;

	/* Append to hint input buffer */
	if (avy->hint_input_len >= 2)
		return false; /* Already full, shouldn't happen */

	avy->hint_input[avy->hint_input_len++] = c;
	avy->hint_input[avy->hint_input_len] = '\0';

	candidates = 0;
	last_candidate = -1;
	for (i = 0; i < avy->match_count; i++) {
		if (strncmp(avy->matches[i].hint,
			    avy->hint_input,
			    (size_t)avy->hint_input_len) == 0) {
			candidates++;
			last_candidate = i;
		}
	}

	if (candidates == 1) {
		/* Unique match found */
		avy->selected_match = last_candidate;
		return true;
	} else if (candidates == 0) {
		/* No matches - reset hint input (typo recovery) */
		avy->hint_input_len = 0;
		avy->hint_input[0] = '\0';
	}
	/* else: multiple candidates, wait for more input */

	return false;
}

struct avy_match *
avy_get_selected(struct avy_state *avy)
{
	if (avy->selected_match < 0 || avy->selected_match >= avy->match_count)
		return NULL;
	return &avy->matches[avy->selected_match];
}
void
avy_draw_hints(struct ui_ctx *ctx,
	       struct avy_state *avy,
	       int *line_y_positions,
	       int line_count,
	       int first_visible_line,
	       int cursor_line,
	       int padding_x)
{
	int i;
	int line_idx;
	int x, y;
	int char_w, line_h;
	int hint_len, hint_w;
	struct avy_match *m;
	ui_rect bg;

	/* Get character width (monospace assumption) */
	char_w = font_char_index_to_x(ctx->render.font, STR_LIT("M"), 1);
	line_h = font_get_line_height(ctx->render.font);

	for (i = 0; i < avy->match_count; i++) {
		m = &avy->matches[i];

		/* Map buffer line to visible line index */
		line_idx = m->line - first_visible_line;
		if (m->line > cursor_line)
			line_idx--; /* Account for cursor line gap (input box) */
		if (line_idx < 0 || line_idx >= line_count)
			continue;

		/* Calculate pixel position */
		y = line_y_positions[line_idx];
		x = padding_x + (m->col * char_w);

		/* Draw hint background (contrasting box) */
		hint_len = (int)strlen(m->hint);
		hint_w = hint_len * char_w + 4;
		bg.x = x - 2;
		bg.y = y;
		bg.w = hint_w;
		bg.h = line_h;
		draw_rect(&ctx->render, bg, ctx->theme.bg_active);

		/* Draw hint text in blue */
		ui_label_draw_colored(
		    ctx, x, y, str_from_cstr(m->hint), ZENBURN_BLUE);
	}
}
