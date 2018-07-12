/* Measure memcpy functions.
   Copyright (C) 2013-2017 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef MEMCPY_RESULT
# define MEMCPY_RESULT(dst, len) dst
# define MIN_PAGE_SIZE 131072
# define TEST_MAIN
# define TEST_NAME "memcpy"
# include "bench-string.h"

char *simple_memcpy (char *, const char *, size_t);
char *builtin_memcpy (char *, const char *, size_t);

IMPL (simple_memcpy, 0)
IMPL (builtin_memcpy, 0)
IMPL (memcpy, 1)

char *
simple_memcpy (char *dst, const char *src, size_t n)
{
  char *ret = dst;
  while (n--)
    *dst++ = *src++;
  return ret;
}

char *
builtin_memcpy (char *dst, const char *src, size_t n)
{
  return __builtin_memcpy (dst, src, n);
}
#endif

# include "json-lib.h"

typedef char *(*proto_t) (char *, const char *, size_t);

static void
do_one_test (json_ctx_t *json_ctx, impl_t *impl, char *dst, const char *src,
	     size_t len)
{
  size_t i, iters = INNER_LOOP_ITERS;
  timing_t start, stop, cur;

  /* Must clear the destination buffer set by the previous run.  */
  for (i = 0; i < len; i++)
    dst[i] = 0;

  if (CALL (impl, dst, src, len) != MEMCPY_RESULT (dst, len))
    {
      error (0, 0, "Wrong result in function %s %p %p", impl->name,
	     CALL (impl, dst, src, len), MEMCPY_RESULT (dst, len));
      ret = 1;
      return;
    }

  if (memcmp (dst, src, len) != 0)
    {
      error (0, 0, "Wrong result in function %s dst \"%s\" src \"%s\"",
	     impl->name, dst, src);
      ret = 1;
      return;
    }

  TIMING_NOW (start);
  for (i = 0; i < iters; ++i)
    {
      CALL (impl, dst, src, len);
    }
  TIMING_NOW (stop);

  TIMING_DIFF (cur, start, stop);

  json_element_double (json_ctx, (double) cur / (double) iters);
}

static void
do_test (json_ctx_t *json_ctx, size_t align1, size_t align2, size_t len)
{
  size_t i, j;
  char *s1, *s2;

  align1 &= 63;
  if (align1 + len >= page_size)
    return;

  align2 &= 63;
  if (align2 + len >= page_size)
    return;

  s1 = (char *) (buf1 + align1);
  s2 = (char *) (buf2 + align2);

  for (i = 0, j = 1; i < len; i++, j += 23)
    s1[i] = j;

  json_element_object_begin (json_ctx);
  json_attr_uint (json_ctx, "length", (double) len);
  json_attr_uint (json_ctx, "align1", (double) align1);
  json_attr_uint (json_ctx, "align2", (double) align2);
  json_array_begin (json_ctx, "timings");

  FOR_EACH_IMPL (impl, 0)
    do_one_test (json_ctx, impl, s2, s1, len);

  json_array_end (json_ctx);
  json_element_object_end (json_ctx);
}

int
test_main (void)
{
  json_ctx_t json_ctx;
  size_t i;

  test_init ();

  json_init (&json_ctx, 0, stdout);

  json_document_begin (&json_ctx);
  json_attr_string (&json_ctx, "timing_type", TIMING_TYPE);

  json_attr_object_begin (&json_ctx, "functions");
  json_attr_object_begin (&json_ctx, "memcpy");
  json_attr_string (&json_ctx, "bench-variant", "default");

  json_array_begin (&json_ctx, "ifuncs");
  FOR_EACH_IMPL (impl, 0)
    json_element_string (&json_ctx, impl->name);
  json_array_end (&json_ctx);

  json_array_begin (&json_ctx, "results");
  for (i = 0; i < 18; ++i)
    {
      do_test (&json_ctx, 0, 0, 1 << i);
      do_test (&json_ctx, i, 0, 1 << i);
      do_test (&json_ctx, 0, i, 1 << i);
      do_test (&json_ctx, i, i, 1 << i);
    }

  for (i = 0; i < 32; ++i)
    {
      do_test (&json_ctx, 0, 0, i);
      do_test (&json_ctx, i, 0, i);
      do_test (&json_ctx, 0, i, i);
      do_test (&json_ctx, i, i, i);
    }

  for (i = 3; i < 32; ++i)
    {
      if ((i & (i - 1)) == 0)
	continue;
      do_test (&json_ctx, 0, 0, 16 * i);
      do_test (&json_ctx, i, 0, 16 * i);
      do_test (&json_ctx, 0, i, 16 * i);
      do_test (&json_ctx, i, i, 16 * i);
    }

  for (i = 32; i < 64; ++i)
    {
      do_test (&json_ctx, 0, 0, 32 * i);
      do_test (&json_ctx, i, 0, 32 * i);
      do_test (&json_ctx, 0, i, 32 * i);
      do_test (&json_ctx, i, i, 32 * i);
    }

  do_test (&json_ctx, 0, 0, getpagesize ());

  json_array_end (&json_ctx);
  json_attr_object_end (&json_ctx);
  json_attr_object_end (&json_ctx);
  json_document_end (&json_ctx);

  return ret;
}

#include <support/test-driver.c>
