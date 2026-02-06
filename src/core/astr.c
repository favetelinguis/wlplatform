#include <core/astr.h>
#include <stdio.h>
#include <string.h>

struct str
astr_from_cstr(struct arena *a, const char *s)
{
	int len;
	char *p;

	if (!s)
		return (struct str){NULL, 0};

	len = (int)strlen(s);
	p = arena_alloc(a, (size_t)len + 1, 1);
	memcpy(p, s, (size_t)len + 1);
	return (struct str){p, len};
}

struct str
astr_from_str(struct arena *a, struct str s)
{
	char *p;

	if (!s.data || s.len == 0)
		return (struct str){NULL, 0};

	p = arena_alloc(a, (size_t)s.len + 1, 1);
	memcpy(p, s.data, (size_t)s.len);
	p[s.len] = '\0';
	return (struct str){p, s.len};
}

struct str
astr_vfmt(struct arena *a, const char *fmt, va_list ap)
{
	va_list ap2;
	int len;
	char *p;

	va_copy(ap2, ap);
	len = vsnprintf(NULL, 0, fmt, ap2);
	va_end(ap2);

	if (len < 0)
		return (struct str){NULL, 0};

	p = arena_alloc(a, (size_t)len + 1, 1);
	vsnprintf(p, (size_t)len + 1, fmt, ap);
	return (struct str){p, len};
}

struct str
astr_fmt(struct arena *a, const char *fmt, ...)
{
	va_list ap;
	struct str result;

	va_start(ap, fmt);
	result = astr_vfmt(a, fmt, ap);
	va_end(ap);
	return result;
}

struct str
astr_cat(struct arena *a, struct str s1, struct str s2)
{
	int len;
	char *p;

	len = s1.len + s2.len;
	p = arena_alloc(a, (size_t)len + 1, 1);

	if (s1.len)
		memcpy(p, s1.data, (size_t)s1.len);
	if (s2.len)
		memcpy(p + s1.len, s2.data, (size_t)s2.len);
	p[len] = '\0';

	return (struct str){p, len};
}

struct str
astr_cat3(struct arena *a, struct str s1, struct str s2, struct str s3)
{
	int len;
	char *p, *dst;

	len = s1.len + s2.len + s3.len;
	p = arena_alloc(a, (size_t)len + 1, 1);
	dst = p;

	if (s1.len) {
		memcpy(dst, s1.data, (size_t)s1.len);
		dst += s1.len;
	}
	if (s2.len) {
		memcpy(dst, s2.data, (size_t)s2.len);
		dst += s2.len;
	}
	if (s3.len) {
		memcpy(dst, s3.data, (size_t)s3.len);
		dst += s3.len;
	}
	*dst = '\0';

	return (struct str){p, len};
}

struct str
astr_substr(struct arena *a, struct str s, int start, int len)
{
	char *p;

	if (start < 0)
		start = 0;
	if (start >= s.len)
		return (struct str){NULL, 0};
	if (start + len > s.len)
		len = s.len - start;

	p = arena_alloc(a, (size_t)len + 1, 1);
	memcpy(p, s.data + start, (size_t)len);
	p[len] = '\0';
	return (struct str){p, len};
}

struct str
astr_join(struct arena *a, struct str sep, struct str *parts, int count)
{
	int total, i;
	char *p, *dst;

	if (count == 0)
		return (struct str){NULL, 0};
	if (count == 1)
		return astr_from_str(a, parts[0]);

	/* Calculate total length */
	total = 0;
	for (i = 0; i < count; i++)
		total += parts[i].len;
	total += sep.len * (count - 1);

	p = arena_alloc(a, (size_t)total + 1, 1);
	dst = p;

	for (i = 0; i < count; i++) {
		if (i > 0 && sep.len) {
			memcpy(dst, sep.data, (size_t)sep.len);
			dst += sep.len;
		}
		if (parts[i].len) {
			memcpy(dst, parts[i].data, (size_t)parts[i].len);
			dst += parts[i].len;
		}
	}
	*dst = '\0';

	return (struct str){p, total};
}

struct str
astr_path_join(struct arena *a, struct str dir, struct str file)
{
	if (dir.len == 0)
		return astr_from_str(a, file);
	if (file.len == 0)
		return astr_from_str(a, dir);

	/* Check if dir already ends with / */
	if (dir.data[dir.len - 1] == '/')
		return astr_cat(a, dir, file);

	return astr_cat3(a, dir, STR_LIT("/"), file);
}
