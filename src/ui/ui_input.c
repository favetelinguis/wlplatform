#include <ui/ui_input.h>

#include <string.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <platform/platform.h> /* For MOD_CTRL, MOD_ALT */

/* ============================================================
 * LIFECYCLE
 * ============================================================ */

void
ui_input_init(struct ui_input *input)
{
	memset(input, 0, sizeof(*input));
}

void
ui_input_set_text(struct ui_input *input, const char *text)
{
	int len;

	if (!text) {
		ui_input_init(input);
		return;
	}

	len = strlen(text);
	if (len > UI_INPUT_MAX_LEN)
		len = UI_INPUT_MAX_LEN;

	memcpy(input->buf, text, len);
	input->buf[len] = '\0';
	input->len = len;
	input->cursor = len; /* Cursor at end */
	input->scroll_offset = 0;
}

const char *
ui_input_get_text(struct ui_input *input)
{
	return input->buf;
}

/* ============================================================
 * WORD BOUNDARY HELPERS
 * ============================================================ */

static bool
is_word_char(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
	       (c >= '0' && c <= '9') || c == '_';
}

static int
find_word_end(const char *buf, int pos, int len)
{
	while (pos < len && !is_word_char(buf[pos]))
		pos++;
	while (pos < len && is_word_char(buf[pos]))
		pos++;
	return pos;
}

static int
find_word_start(const char *buf, int pos)
{
	while (pos > 0 && !is_word_char(buf[pos - 1]))
		pos--;
	while (pos > 0 && is_word_char(buf[pos - 1]))
		pos--;
	return pos;
}

/* ============================================================
 * TEXT MANIPULATION
 * ============================================================ */

static bool
input_insert_char(struct ui_input *in, char c)
{
	if (in->len >= UI_INPUT_MAX_LEN)
		return false;
	if (c < 32 || c > 126)
		return false;

	memmove(&in->buf[in->cursor + 1],
		&in->buf[in->cursor],
		in->len - in->cursor + 1);
	in->buf[in->cursor] = c;
	in->cursor++;
	in->len++;
	return true;
}

static bool
input_delete_char(struct ui_input *in)
{
	if (in->cursor >= in->len)
		return false;

	memmove(&in->buf[in->cursor],
		&in->buf[in->cursor + 1],
		in->len - in->cursor);
	in->len--;
	return true;
}

static bool
input_backspace(struct ui_input *in)
{
	if (in->cursor == 0)
		return false;

	in->cursor--;
	return input_delete_char(in);
}

static bool
input_kill_line(struct ui_input *in)
{
	if (in->cursor >= in->len)
		return false;

	in->buf[in->cursor] = '\0';
	in->len = in->cursor;
	return true;
}

static bool
input_kill_line_back(struct ui_input *in)
{
	if (in->cursor == 0)
		return false;

	memmove(&in->buf[0], &in->buf[in->cursor], in->len - in->cursor + 1);
	in->len -= in->cursor;
	in->cursor = 0;
	return true;
}

static bool
input_kill_word_back(struct ui_input *in)
{
	int new_pos;

	if (in->cursor == 0)
		return false;

	new_pos = find_word_start(in->buf, in->cursor);
	memmove(
	    &in->buf[new_pos], &in->buf[in->cursor], in->len - in->cursor + 1);
	in->len -= (in->cursor - new_pos);
	in->cursor = new_pos;
	return true;
}

static bool
input_kill_word(struct ui_input *in)
{
	int end_pos;

	if (in->cursor >= in->len)
		return false;

	end_pos = find_word_end(in->buf, in->cursor, in->len);
	memmove(
	    &in->buf[in->cursor], &in->buf[end_pos], in->len - end_pos + 1);
	in->len -= (end_pos - in->cursor);
	return true;
}

/* ============================================================
 * KEY HANDLING
 * ============================================================ */

bool
ui_input_handle_key(struct ui_input *in,
		    uint32_t keysym,
		    uint32_t mods,
		    uint32_t codepoint)
{
	bool ctrl = (mods & MOD_CTRL) != 0;
	bool alt = (mods & MOD_ALT) != 0;

	/* Ctrl bindings */
	if (ctrl && !alt) {
		switch (keysym) {
		case XKB_KEY_a:
			if (in->cursor == 0)
				return false;
			in->cursor = 0;
			return true;
		case XKB_KEY_e:
			if (in->cursor == in->len)
				return false;
			in->cursor = in->len;
			return true;
		case XKB_KEY_f:
			if (in->cursor >= in->len)
				return false;
			in->cursor++;
			return true;
		case XKB_KEY_b:
			if (in->cursor == 0)
				return false;
			in->cursor--;
			return true;
		case XKB_KEY_d:
			return input_delete_char(in);
		case XKB_KEY_h:
			return input_backspace(in);
		case XKB_KEY_k:
			return input_kill_line(in);
		case XKB_KEY_u:
			return input_kill_line_back(in);
		case XKB_KEY_w:
			return input_kill_word_back(in);
		default:
			return false;
		}
	}

	/* Alt bindings */
	if (alt && !ctrl) {
		switch (keysym) {
		case XKB_KEY_f:
			if (in->cursor >= in->len)
				return false;
			in->cursor =
			    find_word_end(in->buf, in->cursor, in->len);
			return true;
		case XKB_KEY_b:
			if (in->cursor == 0)
				return false;
			in->cursor = find_word_start(in->buf, in->cursor);
			return true;
		case XKB_KEY_d:
			return input_kill_word(in);
		default:
			return false;
		}
	}

	/* No modifiers (or just Shift for uppercase) */
	if (!ctrl && !alt) {
		switch (keysym) {
		case XKB_KEY_Left:
			if (in->cursor == 0)
				return false;
			in->cursor--;
			return true;
		case XKB_KEY_Right:
			if (in->cursor >= in->len)
				return false;
			in->cursor++;
			return true;
		case XKB_KEY_Home:
			if (in->cursor == 0)
				return false;
			in->cursor = 0;
			return true;
		case XKB_KEY_End:
			if (in->cursor == in->len)
				return false;
			in->cursor = in->len;
			return true;
		case XKB_KEY_BackSpace:
			return input_backspace(in);
		case XKB_KEY_Delete:
			return input_delete_char(in);
		default:
			break;
		}

		/* Printable ASCII */
		if (codepoint >= 32 && codepoint <= 126)
			return input_insert_char(in, (char)codepoint);
	}

	return false;
}
