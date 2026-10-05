#pragma once
#include <stddef.h>
class Preferences {
public:
    bool begin(const char *name, bool readOnly = false, const char *partition_label = NULL);
    size_t putBytes(const char *key, const void *value, size_t len);
    size_t getBytesLength(const char *key);
    size_t getBytes(const char *key, void *buf, size_t maxLen);
};
