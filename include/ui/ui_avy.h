/* include/ui/ui_avy.h
 *
 * Avy (jump-to-char) mode.
 * No dependency on buffer - takes str *lines directly.
 */

#ifndef UI_AVY_H
#define UI_AVY_H

#include <stdbool.h>
#include <stdint.h>

#include <core/str.h>
#include <ui/ui_types.h>

#define AVY_MAX_MATCHES 256
#define AVY_HINT_CHARS	"asdfjklgh"

enum avy_direction {
	AVY_DIR_UP,
	AVY_DIR_DOWN,
};

struct avy_match {
	int line;
	int col;
	char hint[4]; /* 1-2 chars + NUL */
};

struct avy_state {
	bool active;
	enum avy_direction direction;
	char search_char;
	struct avy_match matches[AVY_MAX_MATCHES];
	int match_count;
	char hint_input[4];
	int hint_input_len;
	int selected_match; /* -1 = none */
};

void avy_init(struct avy_state *avy);
void avy_start(struct avy_state *avy, enum avy_direction dir);
void avy_cancel(struct avy_state *avy);

/* Takes lines array directly instead of buffer */
void avy_set_char(struct avy_state *avy,
		  char c,
		  const struct str *lines,
		  int line_count,
		  int cursor_line,
		  int first_visible,
		  int last_visible);

bool avy_input_hint(struct avy_state *avy, char c);
struct avy_match *avy_get_selected(struct avy_state *avy);

void avy_draw_hints(struct ui_ctx *ctx,
		    struct avy_state *avy,
		    int *line_y_positions,
		    int line_count,
		    int first_visible_line,
		    int cursor_line,
		    int padding_x);

#endif /* UI_AVY_H */
