#ifndef PISOUND_MICRO_CONFIG_SCHEMA_H
#define PISOUND_MICRO_CONFIG_SCHEMA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char *get_config_schema(void);
extern size_t get_config_schema_length(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PISOUND_MICRO_CONFIG_SCHEMA_H
