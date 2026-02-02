#include "str.h"
#include "str_buf.h"
#include "test_framework.h"
#include <string.h>

// Domain-specific assertion for str equality with detailed output
#define ASSERT_STR_EQ(a, b) do { \
    str _a = (a), _b = (b); \
    if (!str_eq(_a, _b)) { \
        fprintf(stderr, "FAIL: %s:%d: strings not equal\n", __FILE__, __LINE__); \
        fprintf(stderr, "  left:  \"%.*s\" (len=%zu)\n", (int)_a.len, _a.data, _a.len); \
        fprintf(stderr, "  right: \"%.*s\" (len=%zu)\n", (int)_b.len, _b.data, _b.len); \
        exit(1); \
    } \
} while(0)

// === Category A: Construction & Basic Operations === //

TEST(str_from_cstr_basic) {
    str s = str_from_cstr("hello");
    ASSERT_EQ(str_len(s), 5, "length should be 5");
    ASSERT(!str_empty(s), "should not be empty");
    ASSERT_STR_EQ(s, STR_LIT("hello"));
}

TEST(str_from_cstr_empty) {
    str s = str_from_cstr("");
    ASSERT_EQ(str_len(s), 0, "length should be 0");
    ASSERT(str_empty(s), "should be empty");
}

TEST(str_from_cstr_null) {
    str s = str_from_cstr(NULL);
    ASSERT_EQ(str_len(s), 0, "null should give empty string");
    ASSERT(str_empty(s), "null should be empty");
}

TEST(str_from_parts) {
    const char *data = "hello world";
    str s = str_from_parts(data, 5);
    ASSERT_EQ(str_len(s), 5, "length should be 5");
    ASSERT_STR_EQ(s, STR_LIT("hello"));
}

TEST(str_lit_macro) {
    str s = STR_LIT("test");
    ASSERT_EQ(str_len(s), 4, "STR_LIT should compute length");
    ASSERT_STR_EQ(s, str_from_cstr("test"));
}

TEST(str_empty_constant) {
    ASSERT_EQ(str_len(STR_EMPTY), 0, "STR_EMPTY should have length 0");
    ASSERT(str_empty(STR_EMPTY), "STR_EMPTY should be empty");
}

// === Category B: Memory Safety & Edge Cases === //

TEST(str_at_valid) {
    str s = STR_LIT("hello");
    str_char_opt ch = str_at(s, 0);
    ASSERT(ch.valid, "index 0 should be valid");
    ASSERT_EQ(ch.value, 'h', "should be 'h'");

    ch = str_at(s, 4);
    ASSERT(ch.valid, "index 4 should be valid");
    ASSERT_EQ(ch.value, 'o', "should be 'o'");
}

TEST(str_at_invalid) {
    str s = STR_LIT("hello");
    str_char_opt ch = str_at(s, 5);
    ASSERT(!ch.valid, "index 5 should be invalid");

    ch = str_at(s, 100);
    ASSERT(!ch.valid, "index 100 should be invalid");
}

TEST(str_at_empty) {
    str_char_opt ch = str_at(STR_EMPTY, 0);
    ASSERT(!ch.valid, "empty string has no valid indices");
}

TEST(str_slice_basic) {
    str s = STR_LIT("hello world");
    str sub = str_slice(s, 0, 5);
    ASSERT_STR_EQ(sub, STR_LIT("hello"));

    sub = str_slice(s, 6, 11);
    ASSERT_STR_EQ(sub, STR_LIT("world"));
}

TEST(str_slice_out_of_bounds) {
    str s = STR_LIT("hello");
    str sub = str_slice(s, 10, 20);
    ASSERT(str_empty(sub), "slice beyond bounds should be empty");

    sub = str_slice(s, 3, 100);
    ASSERT_STR_EQ(sub, STR_LIT("lo"));
}

TEST(str_slice_inverted) {
    str s = STR_LIT("hello");
    str sub = str_slice(s, 3, 1);
    ASSERT(str_empty(sub), "inverted slice should be empty");
}

// === Category C: Correctness - Equality & Comparison === //

TEST(str_eq_same) {
    str a = STR_LIT("hello");
    str b = STR_LIT("hello");
    ASSERT(str_eq(a, b), "same strings should be equal");
}

TEST(str_eq_different) {
    str a = STR_LIT("hello");
    str b = STR_LIT("world");
    ASSERT(!str_eq(a, b), "different strings should not be equal");
}

TEST(str_eq_different_length) {
    str a = STR_LIT("hello");
    str b = STR_LIT("hello world");
    ASSERT(!str_eq(a, b), "different lengths should not be equal");
}

TEST(str_eq_empty) {
    ASSERT(str_eq(STR_EMPTY, STR_EMPTY), "empty strings should be equal");
}

TEST(str_cmp_ordering) {
    str a = STR_LIT("apple");
    str b = STR_LIT("banana");
    str c = STR_LIT("apple");

    ASSERT(str_cmp(a, b) < 0, "apple < banana");
    ASSERT(str_cmp(b, a) > 0, "banana > apple");
    ASSERT(str_cmp(a, c) == 0, "apple == apple");
}

TEST(str_cmp_length) {
    str a = STR_LIT("abc");
    str b = STR_LIT("abcd");
    ASSERT(str_cmp(a, b) < 0, "shorter string is less");
    ASSERT(str_cmp(b, a) > 0, "longer string is greater");
}

// === Category C: Correctness - String Matching === //

TEST(str_starts_with_true) {
    str s = STR_LIT("hello world");
    ASSERT(str_starts_with(s, STR_LIT("hello")), "should start with 'hello'");
    ASSERT(str_starts_with(s, STR_LIT("h")), "should start with 'h'");
    ASSERT(str_starts_with(s, STR_LIT("")), "should start with empty string");
}

TEST(str_starts_with_false) {
    str s = STR_LIT("hello world");
    ASSERT(!str_starts_with(s, STR_LIT("world")), "should not start with 'world'");
    ASSERT(!str_starts_with(s, STR_LIT("hello world!")), "prefix too long");
}

TEST(str_ends_with_true) {
    str s = STR_LIT("hello world");
    ASSERT(str_ends_with(s, STR_LIT("world")), "should end with 'world'");
    ASSERT(str_ends_with(s, STR_LIT("d")), "should end with 'd'");
    ASSERT(str_ends_with(s, STR_LIT("")), "should end with empty string");
}

TEST(str_ends_with_false) {
    str s = STR_LIT("hello world");
    ASSERT(!str_ends_with(s, STR_LIT("hello")), "should not end with 'hello'");
    ASSERT(!str_ends_with(s, STR_LIT("hello world!")), "suffix too long");
}

// === Category C: Correctness - Search === //

TEST(str_find_basic) {
    str s = STR_LIT("hello world");
    str_search_result r = str_find(s, STR_LIT("world"));
    ASSERT(r.found, "should find 'world'");
    ASSERT_EQ(r.index, 6, "should be at index 6");
}

TEST(str_find_not_found) {
    str s = STR_LIT("hello world");
    str_search_result r = str_find(s, STR_LIT("xyz"));
    ASSERT(!r.found, "should not find 'xyz'");
}

TEST(str_find_empty_needle) {
    str s = STR_LIT("hello");
    str_search_result r = str_find(s, STR_LIT(""));
    ASSERT(r.found, "empty needle should be found");
    ASSERT_EQ(r.index, 0, "should be at index 0");
}

TEST(str_find_in_empty) {
    str_search_result r = str_find(STR_EMPTY, STR_LIT("x"));
    ASSERT(!r.found, "should not find in empty string");
}

TEST(str_rfind_basic) {
    str s = STR_LIT("hello hello");
    str_search_result r = str_rfind(s, STR_LIT("hello"));
    ASSERT(r.found, "should find 'hello'");
    ASSERT_EQ(r.index, 6, "should be at last occurrence");
}

TEST(str_rfind_empty_needle) {
    str s = STR_LIT("hello");
    str_search_result r = str_rfind(s, STR_LIT(""));
    ASSERT(r.found, "empty needle should be found");
    ASSERT_EQ(r.index, 5, "should be at end");
}

// === Category D: str_buf Operations === //

TEST(str_buf_new) {
    str_buf *sb = str_buf_new();
    ASSERT(sb != NULL, "should allocate");
    ASSERT_EQ(str_buf_len(sb), 0, "should start empty");
    ASSERT(str_buf_capacity(sb) >= 16, "should have initial capacity");
    str_buf_free(sb);
}

TEST(str_buf_with_capacity) {
    str_buf *sb = str_buf_with_capacity(100);
    ASSERT(sb != NULL, "should allocate");
    ASSERT_EQ(str_buf_len(sb), 0, "should start empty");
    ASSERT(str_buf_capacity(sb) >= 100, "should have requested capacity");
    str_buf_free(sb);
}

TEST(str_buf_push) {
    str_buf *sb = str_buf_new();
    str_buf_push(sb, 'h');
    str_buf_push(sb, 'i');
    ASSERT_EQ(str_buf_len(sb), 2, "should have length 2");
    ASSERT_STR_EQ(str_buf_view(sb), STR_LIT("hi"));
    str_buf_free(sb);
}

TEST(str_buf_append) {
    str_buf *sb = str_buf_new();
    str_buf_append(sb, STR_LIT("hello"));
    str_buf_append(sb, STR_LIT(" "));
    str_buf_append(sb, STR_LIT("world"));
    ASSERT_EQ(str_buf_len(sb), 11, "should have length 11");
    ASSERT_STR_EQ(str_buf_view(sb), STR_LIT("hello world"));
    str_buf_free(sb);
}

TEST(str_buf_append_cstr) {
    str_buf *sb = str_buf_new();
    str_buf_append_cstr(sb, "test");
    ASSERT_STR_EQ(str_buf_view(sb), STR_LIT("test"));
    str_buf_free(sb);
}

TEST(str_buf_growth) {
    str_buf *sb = str_buf_new();
    // Append enough to force reallocation
    for (int i = 0; i < 100; i++) {
        str_buf_push(sb, 'x');
    }
    ASSERT_EQ(str_buf_len(sb), 100, "should have length 100");
    ASSERT(str_buf_capacity(sb) >= 100, "should have grown");
    str_buf_free(sb);
}

TEST(str_buf_clear) {
    str_buf *sb = str_buf_new();
    str_buf_append_cstr(sb, "hello");
    size_t cap = str_buf_capacity(sb);
    str_buf_clear(sb);
    ASSERT_EQ(str_buf_len(sb), 0, "should be empty after clear");
    ASSERT_EQ(str_buf_capacity(sb), cap, "capacity should be unchanged");
    str_buf_free(sb);
}

TEST(str_buf_repeated_clear) {
    str_buf *sb = str_buf_new();
    for (int i = 0; i < 10; i++) {
        str_buf_append_cstr(sb, "test");
        str_buf_clear(sb);
    }
    ASSERT_EQ(str_buf_len(sb), 0, "should be empty");
    str_buf_free(sb);
}

TEST(str_buf_reserve) {
    str_buf *sb = str_buf_new();
    str_buf_reserve(sb, 1000);
    ASSERT(str_buf_capacity(sb) >= 1000, "should have reserved space");
    str_buf_free(sb);
}

// === Category E: Conversion & Interop === //

TEST(str_to_cstr) {
    str s = STR_LIT("hello");
    char *cstr = str_to_cstr(s);
    ASSERT(cstr != NULL, "should allocate");
    ASSERT(strcmp(cstr, "hello") == 0, "should match");
    free(cstr);
}

TEST(str_buf_view) {
    str_buf *sb = str_buf_new();
    str_buf_append_cstr(sb, "test");
    str view = str_buf_view(sb);
    ASSERT_STR_EQ(view, STR_LIT("test"));
    str_buf_free(sb);
}

TEST(str_buf_cstr) {
    str_buf *sb = str_buf_new();
    str_buf_append_cstr(sb, "test");
    const char *cstr = str_buf_cstr(sb);
    ASSERT(strcmp(cstr, "test") == 0, "should be null-terminated");
    str_buf_free(sb);
}

TEST(round_trip_conversion) {
    const char *original = "hello world";
    str s = str_from_cstr(original);
    str_buf *sb = str_buf_new();
    str_buf_append(sb, s);
    char *result = str_to_cstr(str_buf_view(sb));
    ASSERT(strcmp(result, original) == 0, "round-trip should preserve");
    free(result);
    str_buf_free(sb);
}

// === Main Test Runner === //

int main(void) {
    printf("Running str library tests...\n\n");

    // Category A: Construction & basic operations
    printf("Category A: Construction & basic operations\n");
    test_str_from_cstr_basic();
    test_str_from_cstr_empty();
    test_str_from_cstr_null();
    test_str_from_parts();
    test_str_lit_macro();
    test_str_empty_constant();
    printf("  ✓ All construction tests passed\n\n");

    // Category B: Memory safety & edge cases
    printf("Category B: Memory safety & edge cases\n");
    test_str_at_valid();
    test_str_at_invalid();
    test_str_at_empty();
    test_str_slice_basic();
    test_str_slice_out_of_bounds();
    test_str_slice_inverted();
    printf("  ✓ All edge case tests passed\n\n");

    // Category C: Correctness
    printf("Category C: Correctness - Equality & Comparison\n");
    test_str_eq_same();
    test_str_eq_different();
    test_str_eq_different_length();
    test_str_eq_empty();
    test_str_cmp_ordering();
    test_str_cmp_length();
    printf("  ✓ All comparison tests passed\n\n");

    printf("Category C: Correctness - String Matching\n");
    test_str_starts_with_true();
    test_str_starts_with_false();
    test_str_ends_with_true();
    test_str_ends_with_false();
    printf("  ✓ All matching tests passed\n\n");

    printf("Category C: Correctness - Search\n");
    test_str_find_basic();
    test_str_find_not_found();
    test_str_find_empty_needle();
    test_str_find_in_empty();
    test_str_rfind_basic();
    test_str_rfind_empty_needle();
    printf("  ✓ All search tests passed\n\n");

    // Category D: Buffer operations
    printf("Category D: Buffer operations\n");
    test_str_buf_new();
    test_str_buf_with_capacity();
    test_str_buf_push();
    test_str_buf_append();
    test_str_buf_append_cstr();
    test_str_buf_growth();
    test_str_buf_clear();
    test_str_buf_repeated_clear();
    test_str_buf_reserve();
    printf("  ✓ All buffer tests passed\n\n");

    // Category E: Conversion & interop
    printf("Category E: Conversion & interop\n");
    test_str_to_cstr();
    test_str_buf_view();
    test_str_buf_cstr();
    test_round_trip_conversion();
    printf("  ✓ All conversion tests passed\n\n");

    printf("========================================\n");
    printf("All tests passed! ✓\n");
    printf("========================================\n");

    return 0;
}
