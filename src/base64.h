#ifndef SRC_BASE64_H_
#define SRC_BASE64_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t base64_encode(const uint8_t* src, size_t len, char* dst);
size_t base64_decode(const char* src, size_t len, uint8_t* dst);

#ifdef __cplusplus
}
#endif

#endif  // SRC_BASE64_H_
