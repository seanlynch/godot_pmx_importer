#pragma once
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

size_t utf16le_utf8_len(const uint8_t *, size_t);
ssize_t utf16le_to_utf8(const uint8_t *, size_t, char *, size_t);
