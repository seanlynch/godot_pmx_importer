#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

static int_fast32_t decode_utf16le(const uint8_t *data, size_t *offset, size_t size) {
  if (*offset + 2 > size) return -1;
  uint_fast16_t w1 = data[(*offset)++];
  w1 |= data[(*offset)++] << 8;
  if (w1 < 0xd800 || w1 >= 0xdc00) {
    /* This will also accept an unpaired low surrogate */
    return w1;
  }

  if (*offset + 2 > size) {
    return w1;
  }

  uint_fast16_t w2 = data[(*offset)++];
  w2 |= data[(*offset)++] << 8;
  if (w2 < 0xdc00 || w2 > 0xdfff) {
    /* Unpaired high surrogate */
    *offset -= 2;
    return w1;
  }

  return (((int_fast32_t)w1 - 0xd800) << 10 | ((int_fast32_t)w2 - 0xdc00)) + 0x10000;
}

static size_t utf8_len(int_fast32_t codepoint) {
  if (codepoint < 0x80) return 1;
  if (codepoint < 0x800) return 2;
  if (codepoint < 0x10000) return 3;
  return 4;
}

ssize_t utf16le_utf8_len(const uint8_t *data, size_t size) {
  ssize_t length = 0;
  for (size_t offset = 0; offset < size; ) {
    int_fast32_t codepoint = decode_utf16le(data, &offset, size);
    if (codepoint < 0) return -1;
    length += utf8_len(codepoint);
  }
  return length;
}

static ssize_t utf8_write(int_fast32_t codepoint, char *buf, size_t offset, size_t bufsize) {
  size_t length = utf8_len(codepoint);
  if (offset + length >= bufsize) return -1;

  switch (length) {
  case 1:
    *(uint8_t *)&buf[offset] = codepoint;
    break;
  case 2:
    *(uint8_t *)&buf[offset++] = 0xc0 | codepoint >> 6;
    *(uint8_t *)&buf[offset] = 0x80 | (codepoint & 0x3f);
    break;
  case 3:
    if (offset + 2 >= bufsize) return 0;
    *(uint8_t *)&buf[offset++] = 0xe0 | codepoint >> 12;
    *(uint8_t *)&buf[offset++] = 0x80 | ((codepoint >> 6) & 0x3f);
    *(uint8_t *)&buf[offset] = 0x80 | (codepoint & 0x3f);
    break;
  case 4:
    *(uint8_t *)&buf[offset++] = 0xf0 | codepoint >> 18;
    *(uint8_t *)&buf[offset++] = 0x80 | ((codepoint >> 12) & 0x3f);
    *(uint8_t *)&buf[offset++] = 0x80 | ((codepoint >> 6) & 0x3f);
    *(uint8_t *)&buf[offset] = 0x80 | (codepoint & 0x3f);
    break;
  }

  return length;
}

ssize_t utf16le_to_utf8(const uint8_t *data, size_t size, char *buf, size_t bufsize) {
  size_t offset, i;
  for (offset = 0, i = 0; offset < size; ) {
    int_fast32_t codepoint = decode_utf16le(data, &offset, size);
    ssize_t written = utf8_write(codepoint, buf, i, bufsize);
    if (written == -1) break;
    i += written;
  }
  if (i >= bufsize) i = bufsize - 1;
  buf[i++] = '\0';
  return i;
}
