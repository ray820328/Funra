/* Copyright The libuv project and contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "task.h"
#include "../src/idna.c"
#include <string.h>

TEST_IMPL(utf8_decode1) {
  const char* p;
  char b[32];
  int i;

  /* ASCII. */
  p = b;
  snprintf(b, sizeof(b), "%c\x7F", 0x00);
  ASSERT(0 == uv__utf8_decode1(&p, b + sizeof(b)));
  ASSERT(p == b + 1);
  ASSERT(127 == uv__utf8_decode1(&p, b + sizeof(b)));
  ASSERT(p == b + 2);

  /* Two-byte sequences. */
  p = b;
  snprintf(b, sizeof(b), "\xC2\x80\xDF\xBF");
  ASSERT(128 == uv__utf8_decode1(&p, b + sizeof(b)));
  ASSERT(p == b + 2);
  ASSERT(0x7FF == uv__utf8_decode1(&p, b + sizeof(b)));
  ASSERT(p == b + 4);

  /* Three-byte sequences. */
  p = b;
  snprintf(b, sizeof(b), "\xE0\xA0\x80\xEF\xBF\xBF");
  ASSERT(0x800 == uv__utf8_decode1(&p, b + sizeof(b)));
  ASSERT(p == b + 3);
  ASSERT(0xFFFF == uv__utf8_decode1(&p, b + sizeof(b)));
  ASSERT(p == b + 6);

  /* Four-byte sequences. */
  p = b;
  snprintf(b, sizeof(b), "\xF0\x90\x80\x80\xF4\x8F\xBF\xBF");
  ASSERT(0x10000 == uv__utf8_decode1(&p, b + sizeof(b)));
  ASSERT(p == b + 4);
  ASSERT(0x10FFFF == uv__utf8_decode1(&p, b + sizeof(b)));
  ASSERT(p == b + 8);

  /* Four-byte sequences > U+10FFFF; disallowed. */
  p = b;
  snprintf(b, sizeof(b), "\xF4\x90\xC0\xC0\xF7\xBF\xBF\xBF");
  ASSERT((unsigned) -1 == uv__utf8_decode1(&p, b + sizeof(b)));
  ASSERT(p == b + 4);
  ASSERT((unsigned) -1 == uv__utf8_decode1(&p, b + sizeof(b)));
  ASSERT(p == b + 8);

  /* Overlong; disallowed. */
  p = b;
  snprintf(b, sizeof(b), "\xC0\x80\xC1\x80");
  ASSERT((unsigned) -1 == uv__utf8_decode1(&p, b + sizeof(b)));
  ASSERT(p == b + 2);
  ASSERT((unsigned) -1 == uv__utf8_decode1(&p, b + sizeof(b)));
  ASSERT(p == b + 4);

  /* Surrogate pairs; disallowed. */
  p = b;
  snprintf(b, sizeof(b), "\xED\xA0\x80\xED\xA3\xBF");
  ASSERT((unsigned) -1 == uv__utf8_decode1(&p, b + sizeof(b)));
  ASSERT(p == b + 3);
  ASSERT((unsigned) -1 == uv__utf8_decode1(&p, b + sizeof(b)));
  ASSERT(p == b + 6);

  /* Simply illegal. */
  p = b;
  snprintf(b, sizeof(b), "\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

  for (i = 1; i <= 8; i++) {
    ASSERT((unsigned) -1 == uv__utf8_decode1(&p, b + sizeof(b)));
    ASSERT(p == b + i);
  }

  return 0;
}

TEST_IMPL(utf8_decode1_overrun) {
  const char* p;
  char b[1];

  /* Single byte. */
  p = b;
  b[0] = 0x7F;
  ASSERT_EQ(0x7F, uv__utf8_decode1(&p, b + 1));
  ASSERT_EQ(p, b + 1);

  /* Multi-byte. */
  p = b;
  b[0] = 0xC0;
  ASSERT_EQ((unsigned) -1, uv__utf8_decode1(&p, b + 1));
  ASSERT_EQ(p, b + 1);

  return 0;
}

/* Doesn't work on z/OS because that platform uses EBCDIC, not ASCII. */
#ifndef __MVS__

#define F(input, err)                                                         \
  do {                                                                        \
    char d[256] = {0};                                                        \
    static const char s[] = "" input "";                                      \
    ASSERT(err == uv__idna_toascii(s, s + sizeof(s) - 1, d, d + sizeof(d)));  \
  } while (0)

#define T(input, expected)                                                    \
  do {                                                                        \
    long n;                                                                   \
    char d1[256] = {0};                                                       \
    char d2[256] = {0};                                                       \
    static const char s[] = "" input "";                                      \
    n = uv__idna_toascii(s, s + sizeof(s) - 1, d1, d1 + sizeof(d1));          \
    ASSERT(n == sizeof(expected));                                            \
    ASSERT(0 == memcmp(d1, expected, n));                                     \
    /* Sanity check: encoding twice should not change the output. */          \
    n = uv__idna_toascii(d1, d1 + strlen(d1), d2, d2 + sizeof(d2));           \
    ASSERT(n == sizeof(expected));                                            \
    ASSERT(0 == memcmp(d2, expected, n));                                     \
    ASSERT(0 == memcmp(d1, d2, sizeof(d2)));                                  \
  } while (0)

TEST_IMPL(idna_toascii) {
  /* Illegal inputs. */
  F("\xC0\x80\xC1\x80", UV_EINVAL);  /* Overlong UTF-8 sequence. */
  F("\xC0\x80\xC1\x80.com", UV_EINVAL);  /* Overlong UTF-8 sequence. */
  /* No conversion. */
  T("", "");
  T(".", ".");
  T(".com", ".com");
  T("example", "example");
  T("example-", "example-");
  T("stra