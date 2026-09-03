#ifndef ECOS_ERROR_H
#define ECOS_ERROR_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t ecos_err_t;

enum {
    ECOS_OK = 0,
    ECOS_ERR_INVALID_ARGUMENT = -1,
    ECOS_ERR_UNSUPPORTED = -2,
    ECOS_ERR_NOT_INITIALIZED = -3,
    ECOS_ERR_IO = -4,
    ECOS_ERR_TIMEOUT = -5,
    ECOS_ERR_BUSY = -6,
    ECOS_ERR_NO_MEMORY = -7,
    ECOS_ERR_NOT_FOUND = -8,
    ECOS_ERR_INVALID_STATE = -9,
    ECOS_ERR_INTERNAL = -10
};

/* Result-returning APIs may use any non-negative value as a success payload. */
bool ecos_result_succeeded(int result);
bool ecos_result_failed(int result);
bool ecos_err_is_known(int result);

/* Returned strings have static storage duration and must not be modified. */
const char *ecos_err_name(ecos_err_t error);
const char *ecos_err_description(ecos_err_t error);

#ifdef __cplusplus
}
#endif

#endif
