#include <editor/syntax.h>

#include <string.h>
#include <tree_sitter/api.h>

#include <core/arena.h>

extern const TSLanguage *tree_sitter_markdown(void);

struct syntax_ctx {
	TSParser *parser;
	TSTree *tree;
};

struct syntax_ctx *
syntax_create(struct arena *a)
{
	struct syntax_ctx *ctx = arena_new0(a, struct syntax_ctx);

	ctx->parser = ts_parser_new();
	if (!ctx->parser)
		return NULL;

	if (!ts_parser_set_language(ctx->parser, tree_sitter_markdown())) {
		ts_parser_delete(ctx->parser);
		return NULL;
	}
	return ctx;
}

void
syntax_destroy(struct syntax_ctx *ctx)
{
	if (!ctx)
		return;
	if (ctx->tree)
		ts_tree_delete(ctx->tree);
	if (ctx->parser)
		ts_parser_delete(ctx->parser);
}

bool
syntax_parse(struct syntax_ctx *ctx, struct str source)
{
	TSTree *new_tree;

	if (!ctx || str_empty(source))
		return false;

	new_tree = ts_parser_parse_string(
	    ctx->parser, NULL, str_data(source), (uint32_t)str_len(source));
	if (!new_tree)
		return false;

	if (ctx->tree)
		ts_tree_delete(ctx->tree);
	ctx->tree = new_tree;

	return true;
}

bool
syntax_has_tree(struct syntax_ctx *ctx)
{
	return ctx && ctx->tree != NULL;
}

/* Zero-copy text extraction using str_slice */
struct str
syntax_node_text(struct syntax_node *node, struct str source)
{
	return str_slice(source, node->start_byte, node->end_byte);
}

/* Recursive helper to collect visible nodes */
static void
collect_nodes(TSNode node,
	      struct str source,
	      uint32_t start_row,
	      uint32_t end_row,
	      int depth,
	      struct syntax_visible *out)
{
	TSPoint start, end;
	uint32_t i, child_count;

	if (out->count >= SYNTAX_VISIBLE_MAX)
		return;

	start = ts_node_start_point(node);
	end = ts_node_end_point(node);

	/* Skip if entirely outside visible range */
	if (end.row < start_row || start.row > end_row)
		return;

	/* Add named nodes only */
	if (ts_node_is_named(node)) {
		struct syntax_node *n = &out->nodes[out->count];

		strncpy(n->type, ts_node_type(node), SYNTAX_NODE_TYPE_MAX - 1);
		n->type[SYNTAX_NODE_TYPE_MAX - 1] = '\0';

		n->start_row = start.row;
		n->start_col = start.column;
		n->end_row = end.row;
		n->end_col = end.column;
		n->start_byte = ts_node_start_byte(node);
		n->end_byte = ts_node_end_byte(node);
		n->depth = depth;
		n->is_named = true;

		/* Store text view for leaf nodes (using str_slice) */
		if (ts_node_child_count(node) == 0) {
			n->text =
			    str_slice(source, n->start_byte, n->end_byte);
		} else {
			n->text = STR_EMPTY;
		}

		out->count++;
	}

	/* Recurse to children */
	child_count = ts_node_child_count(node);
	for (i = 0; i < child_count && out->count < SYNTAX_VISIBLE_MAX; i++) {
		collect_nodes(ts_node_child(node, i),
			      source,
			      start_row,
			      end_row,
			      depth + 1,
			      out);
	}
}

void
syntax_get_visible_nodes(struct syntax_ctx *ctx,
			 struct str source,
			 uint32_t start_row,
			 uint32_t end_row,
			 struct syntax_visible *out)
{
	out->count = 0;

	if (!ctx || !ctx->tree)
		return;

	TSNode root = ts_tree_root_node(ctx->tree);
	collect_nodes(root, source, start_row, end_row, 0, out);
}
