/* Save system: portable byte serialization + platform storage.
 *   native: monterra.sav file
 *   wasm:   browser localStorage (via EM_ASM)
 */
#ifndef SAVE_H
#define SAVE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* pure serialization (testable) */
size_t save_encode(uint8_t *buf, size_t cap); /* 0 on failure */
bool save_decode(const uint8_t *buf, size_t len);

/* platform storage */
bool save_write(void);
bool save_read(void);   /* fs -> g via save_decode */
bool save_exists(void);

#define SAVE_MAX 1024

#endif
