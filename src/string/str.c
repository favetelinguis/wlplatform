#include "str.h"
#include <string.h>
#include <stdlib.h>

// Helper macros - simple versions to stay portable
// For production, could use typeof() with GNU extensions
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

str str_from_cstr(const char *cstr) {
    if (cstr == NULL) {
        return STR_EMPTY;
    }
    return (str){.data = cstr, .len = strlen(cstr)};
}

str str_from_parts(const char *data, size_t len) {
    if (data == NULL || len == 0) {
        return STR_EMPTY;
    }
    return (str){.data = data, .len = len};
}

str str_slice(str s, size_t start, size_t end) {
    // Clamp to valid bounds
    if (start >= s.len) {
        return STR_EMPTY;
    }

    end = MIN(end, s.len);

    if (start >= end) {
        return STR_EMPTY;
    }

    return (str){.data = s.data + start, .len = end - start};
}

str_char_opt str_at(str s, size_t index) {
    if (index >= s.len) {
        return (str_char_opt){.value = 0, .valid = false};
    }
    return (str_char_opt){.value = s.data[index], .valid = true};
}

bool str_eq(str a, str b) {
    if (a.len != b.len) {
        return false;
    }
    if (a.len == 0) {
        return true;  // Both empty
    }
    return memcmp(a.data, b.data, a.len) == 0;
}

int str_cmp(str a, str b) {
    size_t min_len = MIN(a.len, b.len);

    if (min_len > 0) {
        int result = memcmp(a.data, b.data, min_len);
        if (result != 0) {
            return result;
        }
    }

    // If prefixes are equal, shorter string is "less"
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

bool str_starts_with(str s, str prefix) {
    if (prefix.len > s.len) {
        return false;
    }
    if (prefix.len == 0) {
        return true;  // Empty prefix matches anything
    }
    return memcmp(s.data, prefix.data, prefix.len) == 0;
}

bool str_ends_with(str s, str suffix) {
    if (suffix.len > s.len) {
        return false;
    }
    if (suffix.len == 0) {
        return true;  // Empty suffix matches anything
    }
    return memcmp(s.data + (s.len - suffix.len), suffix.data, suffix.len) == 0;
}

str_search_result str_find(str s, str needle) {
    if (needle.len == 0) {
        return (str_search_result){.index = 0, .found = true};
    }
    if (needle.len > s.len) {
        return (str_search_result){.index = 0, .found = false};
    }

    // Simple brute-force search (good enough for most use cases)
    size_t search_len = s.len - needle.len + 1;
    for (size_t i = 0; i < search_len; i++) {
        if (memcmp(s.data + i, needle.data, needle.len) == 0) {
            return (str_search_result){.index = i, .found = true};
        }
    }

    return (str_search_result){.index = 0, .found = false};
}

str_search_result str_rfind(str s, str needle) {
    if (needle.len == 0) {
        return (str_search_result){.index = s.len, .found = true};
    }
    if (needle.len > s.len) {
        return (str_search_result){.index = 0, .found = false};
    }

    // Search backwards
    for (size_t i = s.len - needle.len + 1; i > 0; i--) {
        size_t idx = i - 1;
        if (memcmp(s.data + idx, needle.data, needle.len) == 0) {
            return (str_search_result){.index = idx, .found = true};
        }
    }

    return (str_search_result){.index = 0, .found = false};
}

char *str_to_cstr(str s) {
    // Allocate buffer with space for null terminator
    char *buf = malloc(s.len + 1);
    if (buf == NULL) {
        return NULL;
    }

    if (s.len > 0) {
        memcpy(buf, s.data, s.len);
    }
    buf[s.len] = '\0';

    return buf;
}
