/**
 * @file common.h
 * @brief HAL (Hardware Abstraction Layer) common definitions and utilities
 * 
 * This file contains common data types, macros, and utilities used across
 * all HAL drivers in the ECOS SDK 2.0.
 */

#ifndef COMMON_H__
#define COMMON_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HAL return status codes
 */
typedef enum {
    STATUS_SUCCESS = 0,        /**< Operation successful */
    STATUS_ERROR = -1,         /**< General error */
    STATUS_INVALID_ARG = -2,   /**< Invalid argument */
    STATUS_NOT_SUPPORTED = -3, /**< Feature not supported */
    STATUS_TIMEOUT = -4,       /**< Operation timeout */
    STATUS_BUSY = -5           /**< Resource busy */
} status_t;

/**
 * @brief HAL boolean type
 */
typedef enum {
    FALSE = 0,
    TRUE = 1
} bool_t;

/**
 * @brief HAL interrupt callback function type
 * 
 * @param arg User-defined argument passed to the callback
 */
typedef void (*interrupt_callback_t)(void* arg);

/**
 * @brief HAL driver base structure
 * 
 * All HAL drivers should extend this structure as their first member
 */
typedef struct {
    uint32_t version;       /**< Driver version (MAJOR << 16 | MINOR) */
    const char* name;       /**< Driver name */
} driver_base_t;

/**
 * @brief Macro to define driver version
 */
#define DRIVER_VERSION(major, minor) (((major) << 16) | (minor))

/**
 * @brief Macro to check if a pointer is valid
 */
#define CHECK_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        return STATUS_INVALID_ARG; \
    } \
} while(0)

/**
 * @brief Macro to check if a pointer is valid for void functions
 */
#define CHECK_NULL_VOID(ptr) do { \
    if ((ptr) == NULL) { \
        return; \
    } \
} while(0)

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H__ */