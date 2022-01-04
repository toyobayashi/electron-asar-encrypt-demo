#include "string.h"
#include "base64.h"

static const char table[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// supports regular and URL-safe base64
static const int8_t unbase64_table[256] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -1, -1, -2, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, 62, -1, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
  -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63,
  -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

size_t base64_encode(const uint8_t* src, size_t len, char* dst) {
  size_t slen, dlen;
  unsigned i, k, n, a, b, c;
  if (src == NULL) {
    return 0;
  }

  if (len == -1) {
    slen = strlen((const char*)src);
  } else {
    slen = len;
  }

  dlen = ((slen + 2 - ((slen + 2) % 3)) / 3 * 4);

  if (dst == NULL) {
    return dlen;
  }

  i = 0;
  k = 0;
  n = slen / 3 * 3;

  while (i < n) {
    a = src[i + 0] & 0xff;
    b = src[i + 1] & 0xff;
    c = src[i + 2] & 0xff;

    dst[k + 0] = table[a >> 2];
    dst[k + 1] = table[((a & 3) << 4) | (b >> 4)];
    dst[k + 2] = table[((b & 0x0f) << 2) | (c >> 6)];
    dst[k + 3] = table[c & 0x3f];

    i += 3;
    k += 4;
  }

  if (n != slen) {
    switch (slen - n) {
      case 1:
        a = src[i + 0] & 0xff;
        dst[k + 0] = table[a >> 2];
        dst[k + 1] = table[(a & 3) << 4];
        dst[k + 2] = '=';
        dst[k + 3] = '=';
        break;

      case 2:
        a = src[i + 0] & 0xff;
        b = src[i + 1] & 0xff;
        dst[k + 0] = table[a >> 2];
        dst[k + 1] = table[((a & 3) << 4) | (b >> 4)];
        dst[k + 2] = table[(b & 0x0f) << 2];
        dst[k + 3] = '=';
        break;
    }
  }

  return dlen;
}

static int base64_decode_group_slow(char* const dst, const size_t dstlen,
                              const char* const src, const size_t srclen,
                              size_t* const i, size_t* const k) {
  uint8_t hi;
  uint8_t lo;
  uint8_t c;
#define V(expr)                                                               \
  for (;;) {                                                                  \
    c = src[*i];                                                              \
    lo = unbase64_table[c];                                                   \
    *i += 1;                                                                  \
    if (lo < 64)                                                              \
      break;  /* Legal character. */                                          \
    if (c == '=' || *i >= srclen)                                             \
      return 0;  /* Stop decoding. */                                         \
  }                                                                           \
  expr;                                                                       \
  if (*i >= srclen)                                                           \
    return 0;                                                                 \
  if (*k >= dstlen)                                                           \
    return 0;                                                                 \
  hi = lo;
  V((void)0);
  V(dst[(*k)++] = ((hi & 0x3F) << 2) | ((lo & 0x30) >> 4));
  V(dst[(*k)++] = ((hi & 0x0F) << 4) | ((lo & 0x3C) >> 2));
  V(dst[(*k)++] = ((hi & 0x03) << 6) | ((lo & 0x3F) >> 0));
#undef V
  return 1;  // Continue decoding.
}

size_t base64_decode(const char* src, size_t len, uint8_t* dst) {
  size_t slen, dlen, remainder, size;
  size_t available, max_k, max_i, i, k, v;

  if (src == NULL) {
    return 0;
  }

  if (len == -1) {
    slen = strlen(src);
  } else {
    slen = len;
  }

  if (slen == 0) {
    dlen = 0;
  } else {
    if (src[slen - 1] == '=') slen--;
    if (slen > 0 && src[slen - 1] == '=') slen--;

    size = slen;
    remainder = size % 4;

    size = (size / 4) * 3;
    if (remainder) {
      if (size == 0 && remainder == 1) {
        size = 0;
      } else {
        size += 1 + (remainder == 3);
      }
    }

    dlen = size;
  }

  if (dst == NULL) {
    return dlen;
  }

  available = dlen;
  max_k = available / 3 * 3;
  max_i = slen / 4 * 4;
  i = 0;
  k = 0;
  while (i < max_i && k < max_k) {
     v = unbase64_table[src[i + 0]] << 24 |
        unbase64_table[src[i + 1]] << 16 |
        unbase64_table[src[i + 2]] << 8 |
        unbase64_table[src[i + 3]];
    // If MSB is set, input contains whitespace or is not valid base64.
    if (v & 0x80808080) {
      if (!base64_decode_group_slow((char*)dst, dlen, src, slen, &i, &k))
        return k;
      max_i = i + (slen - i) / 4 * 4;  // Align max_i again.
    } else {
      dst[k + 0] = ((v >> 22) & 0xFC) | ((v >> 20) & 0x03);
      dst[k + 1] = ((v >> 12) & 0xF0) | ((v >> 10) & 0x0F);
      dst[k + 2] = ((v >>  2) & 0xC0) | ((v >>  0) & 0x3F);
      i += 4;
      k += 3;
    }
  }
  if (i < slen && k < dlen) {
    base64_decode_group_slow((char*)dst, dlen, src, slen, &i, &k);
  }
  return k;
}
