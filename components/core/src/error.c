#include "ecos/error.h"

#ifndef CONFIG_ECOS_ERROR_DESCRIPTIONS
#define CONFIG_ECOS_ERROR_DESCRIPTIONS 1
#endif

bool ecos_result_succeeded(int result)
{
    return result >= 0;
}

bool ecos_result_failed(int result)
{
    return result < 0;
}

bool ecos_err_is_known(int result)
{
    return result <= ECOS_OK && result >= ECOS_ERR_INTERNAL;
}

const char *ecos_err_name(ecos_err_t error)
{
    switch (error) {
    case ECOS_OK:
        return "ECOS_OK";
    case ECOS_ERR_INVALID_ARGUMENT:
        return "ECOS_ERR_INVALID_ARGUMENT";
    case ECOS_ERR_UNSUPPORTED:
        return "ECOS_ERR_UNSUPPORTED";
    case ECOS_ERR_NOT_INITIALIZED:
        return "ECOS_ERR_NOT_INITIALIZED";
    case ECOS_ERR_IO:
        return "ECOS_ERR_IO";
    case ECOS_ERR_TIMEOUT:
        return "ECOS_ERR_TIMEOUT";
    case ECOS_ERR_BUSY:
        return "ECOS_ERR_BUSY";
    case ECOS_ERR_NO_MEMORY:
        return "ECOS_ERR_NO_MEMORY";
    case ECOS_ERR_NOT_FOUND:
        return "ECOS_ERR_NOT_FOUND";
    case ECOS_ERR_INVALID_STATE:
        return "ECOS_ERR_INVALID_STATE";
    case ECOS_ERR_INTERNAL:
        return "ECOS_ERR_INTERNAL";
    default:
        return "ECOS_ERR_UNKNOWN";
    }
}

const char *ecos_err_description(ecos_err_t error)
{
#if CONFIG_ECOS_ERROR_DESCRIPTIONS
    switch (error) {
    case ECOS_OK:
        return "success";
    case ECOS_ERR_INVALID_ARGUMENT:
        return "invalid argument";
    case ECOS_ERR_UNSUPPORTED:
        return "operation not supported";
    case ECOS_ERR_NOT_INITIALIZED:
        return "resource not initialized";
    case ECOS_ERR_IO:
        return "input/output failure";
    case ECOS_ERR_TIMEOUT:
        return "operation timed out";
    case ECOS_ERR_BUSY:
        return "resource busy";
    case ECOS_ERR_NO_MEMORY:
        return "insufficient memory";
    case ECOS_ERR_NOT_FOUND:
        return "resource not found";
    case ECOS_ERR_INVALID_STATE:
        return "invalid state";
    case ECOS_ERR_INTERNAL:
        return "internal failure";
    default:
        return "unknown error";
    }
#else
    (void)error;
    return (const char *)0;
#endif
}
