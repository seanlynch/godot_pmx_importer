#pragma once
#include <stddef.h>
#include <unistd.h>

size_t utf16le_utf8_len(char *, size_t);
ssize_t utf16le_to_utf8(char *, size_t, char *, size_t);
