/* include/editor/syntax.h
 *
 * Tree-sitter based syntax parsing.
 * Layer 3 - depends on core/ only.
 */

#ifndef SYNTAX_H
#define SYNTAX_H

#include <stdbool.h>
#include <stdint.h>

#include <core/arena.h>
#include <core/str.h>

struct syntax_ctx; /* Opaque to hide treesitter details */

#define SYNTAX_NODE_TYPE_MAX 32
#define SYNTAX_VISIBLE_MAX   32

struct syntax_node {
	char type[SYNTAX_NODE_TYPE_MAX];
	struct str text; /* View into source (valid while source lives) */
	uint32_t start_row;
	uint32_t start_col;
	uint32_t end_row;
	uint32_t end_col;
	uint32_t start_byte; /* For str_slice into source */
	uint32_t end_byte;
	int depth;
	bool is_named;
};

struct syntax_visible {
	struct syntax_node nodes[SYNTAX_VISIBLE_MAX];
	int count;
};

struct syntax_ctx *syntax_create(struct arena *a);
void syntax_destroy(struct syntax_ctx *ctx);

/* Parse using str - takes view, no copy needed */
bool syntax_parse(struct syntax_ctx *ctx, struct str source);
bool syntax_has_tree(struct syntax_ctx *ctx);

/* Get nodes intersecting row range */
void syntax_get_visible_nodes(struct syntax_ctx *ctx,
			      struct str source,
			      uint32_t start_row,
			      uint32_t end_row,
			      struct syntax_visible *out);

/* Get text for a node using str_slice (zero-copy) */
struct str syntax_node_text(struct syntax_node *node, struct str source);

#endif /* SYNTAX_H */
