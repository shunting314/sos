#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void create_process_and_run(uint8_t* code, uint32_t len);

#ifdef __cplusplus
}
#endif
