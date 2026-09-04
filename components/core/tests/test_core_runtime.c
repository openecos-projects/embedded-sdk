#include "ecos/error.h"
#include "ecos/log.h"

#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    char data[1024];
    size_t size;
    unsigned calls;
} capture_t;

static int capture_writer(void *context, const char *data, size_t size)
{
    capture_t *capture = (capture_t *)context;

    assert(size < sizeof(capture->data));
    memcpy(capture->data, data, size);
    capture->data[size] = '\0';
    capture->size = size;
    ++capture->calls;
    return (int)size;
}

static int short_writer(void *context, const char *data, size_t size)
{
    (void)context;
    (void)data;
    return size == 0u ? 0 : (int)size - 1;
}

static int failing_writer(void *context, const char *data, size_t size)
{
    (void)context;
    (void)data;
    (void)size;
    return ECOS_ERR_TIMEOUT;
}

static ecos_err_t evaluated_error(unsigned *calls)
{
    ++*calls;
    return ECOS_ERR_IO;
}

static int evaluated_result(unsigned *calls, int result)
{
    ++*calls;
    return result;
}

static int return_on_error(int result, unsigned *calls)
{
    ECOS_RETURN_ON_ERROR(evaluated_result(calls, result));
    return 17;
}

static int goto_on_error(int result,
                         unsigned *calls,
                         unsigned *cleanup_calls)
{
    ecos_err_t error = ECOS_OK;

    ECOS_GOTO_ON_ERROR(evaluated_result(calls, result), error, cleanup);
    return 17;

cleanup:
    ++*cleanup_calls;
    return error;
}

static jmp_buf panic_jump;
static capture_t panic_capture;
static unsigned panic_calls;
static unsigned panic_result_evaluations;

static void jump_from_panic(void *context)
{
    (void)context;
    ++panic_calls;
    longjmp(panic_jump, 1);
}

#if CONFIG_ECOS_LOG_LEVEL <= 0
static int evaluated_value(unsigned *calls)
{
    ++*calls;
    return 7;
}
#endif

static void test_error_model(void)
{
    assert(ECOS_OK == 0);
    assert(ECOS_ERR_INVALID_ARGUMENT == -1);
    assert(ECOS_ERR_UNSUPPORTED == -2);
    assert(ECOS_ERR_NOT_INITIALIZED == -3);
    assert(ECOS_ERR_IO == -4);
    assert(ECOS_ERR_TIMEOUT == -5);
    assert(ECOS_ERR_BUSY == -6);
    assert(ECOS_ERR_NO_MEMORY == -7);
    assert(ECOS_ERR_NOT_FOUND == -8);
    assert(ECOS_ERR_INVALID_STATE == -9);
    assert(ECOS_ERR_INTERNAL == -10);

    assert(ecos_result_succeeded(0));
    assert(ecos_result_succeeded(12));
    assert(!ecos_result_succeeded(ECOS_ERR_IO));
    assert(!ecos_result_failed(0));
    assert(ecos_result_failed(ECOS_ERR_IO));
    assert(ecos_result_failed(-37));
    assert(ecos_err_is_known(ECOS_OK));
    assert(ecos_err_is_known(ECOS_ERR_INTERNAL));
    assert(!ecos_err_is_known(1));
    assert(!ecos_err_is_known(-37));
    assert(strcmp(ecos_err_name(ECOS_ERR_IO), "ECOS_ERR_IO") == 0);
    assert(strcmp(ecos_err_name(-37), "ECOS_ERR_UNKNOWN") == 0);
#if CONFIG_ECOS_ERROR_DESCRIPTIONS
    assert(strcmp(ecos_err_description(ECOS_ERR_TIMEOUT),
                  "operation timed out") == 0);
#else
    assert(ecos_err_description(ECOS_ERR_TIMEOUT) == NULL);
#endif
}

static void test_error_control_flow(void)
{
    unsigned calls = 0u;
    unsigned cleanup_calls = 0u;

    assert(return_on_error(ECOS_OK, &calls) == 17);
    assert(calls == 1u);
    assert(return_on_error(9, &calls) == 17);
    assert(calls == 2u);
    assert(return_on_error(ECOS_ERR_TIMEOUT, &calls) == ECOS_ERR_TIMEOUT);
    assert(calls == 3u);

    assert(goto_on_error(ECOS_OK, &calls, &cleanup_calls) == 17);
    assert(calls == 4u);
    assert(cleanup_calls == 0u);
    assert(goto_on_error(-37, &calls, &cleanup_calls) == -37);
    assert(calls == 5u);
    assert(cleanup_calls == 1u);
}

static void test_panic_on_error(void)
{
    memset(&panic_capture, 0, sizeof(panic_capture));
    panic_calls = 0u;
    panic_result_evaluations = 0u;

    assert(ecos_log_set_writer(capture_writer, &panic_capture) == ECOS_OK);
    ecos_panic_set_handler(jump_from_panic, NULL);

    ECOS_PANIC_ON_ERROR(
        "core",
        evaluated_result(&panic_result_evaluations, ECOS_OK),
        "successful operation"
    );
    assert(panic_result_evaluations == 1u);
    assert(panic_calls == 0u);

    if (setjmp(panic_jump) == 0) {
        ECOS_PANIC_ON_ERROR(
            "core",
            evaluated_result(&panic_result_evaluations, ECOS_ERR_IO),
            "failed operation"
        );
        assert(0);
    }

    ecos_panic_set_handler(NULL, NULL);
    assert(panic_result_evaluations == 2u);
    assert(panic_calls == 1u);
    assert(strstr(panic_capture.data,
                  "failed operation: ECOS_ERR_IO (-4)") != NULL);
}

static void test_log_output(void)
{
    capture_t capture = {{0}, 0u, 0u};
    unsigned debug_evaluations = 0u;
    unsigned error_evaluations = 0u;
    int result;

    assert(ecos_log_set_writer(NULL, NULL) == ECOS_OK);
    assert(ecos_log_write(ECOS_LOG_INFO, "core", NULL, 0, "ready") ==
           ECOS_ERR_NOT_INITIALIZED);
    assert(ecos_log_set_writer(NULL, &capture) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_log_set_writer(capture_writer, &capture) == ECOS_OK);
    assert(ecos_log_set_level(ECOS_LOG_INFO) == ECOS_OK);
    assert(ecos_log_get_level() == ECOS_LOG_INFO);

    result = ecos_log_write(ECOS_LOG_INFO, "core", NULL, 0, "value=%d", 7);
#if CONFIG_ECOS_LOG_SOURCE_LOCATION
    assert(result == (int)strlen("[I][core] value=7 @ ?:0\r\n"));
    assert(strcmp(capture.data, "[I][core] value=7 @ ?:0\r\n") == 0);
#else
    assert(result == (int)strlen("[I][core] value=7\r\n"));
    assert(strcmp(capture.data, "[I][core] value=7\r\n") == 0);
#endif

#if CONFIG_ECOS_LOG_LEVEL <= 0
    result = ECOS_LOGD("core", "value=%d",
                       evaluated_value(&debug_evaluations));
#else
    result = ECOS_LOGD("core", "value=%d", ++debug_evaluations);
#endif
    assert(result == ECOS_OK);
#if CONFIG_ECOS_LOG_LEVEL <= 0
    assert(debug_evaluations == 1u);
#else
    assert(debug_evaluations == 0u);
#endif
    assert(capture.calls == 1u);

    result = ecos_log_write(ECOS_LOG_DEBUG, "core", NULL, 0, "filtered");
    assert(result == ECOS_OK);
    assert(capture.calls == 1u);

    result = ECOS_LOG_ERR("storage", evaluated_error(&error_evaluations),
                          "mount");
    assert(result > 0);
    assert(error_evaluations == 1u);
#if CONFIG_ECOS_ERROR_DESCRIPTIONS
    assert(strstr(capture.data,
                  "[E][storage] mount: ECOS_ERR_IO (-4): input/output failure") !=
           NULL);
#else
    assert(strstr(capture.data,
                  "[E][storage] mount: ECOS_ERR_IO (-4)\r\n") != NULL);
#endif

    result = ecos_log_error("storage", -37, "mount", NULL, 0);
    assert(result > 0);
    assert(strstr(capture.data, "ECOS_ERR_UNKNOWN (-37)") != NULL);
    assert(capture.size >= 2u);
    assert(capture.data[capture.size - 2u] == '\r');
    assert(capture.data[capture.size - 1u] == '\n');

#if CONFIG_ECOS_LOG_SOURCE_LOCATION
    result = ecos_log_write(ECOS_LOG_WARN, "core", "/tmp/runtime.c", 42,
                            "located");
    assert(result > 0);
    assert(strcmp(capture.data,
                  "[W][core] located @ runtime.c:42\r\n") == 0);
#endif

    assert(ecos_log_set_writer(short_writer, NULL) == ECOS_OK);
    assert(ecos_log_write(ECOS_LOG_INFO, "core", NULL, 0, "short") ==
           ECOS_ERR_IO);
    assert(ecos_log_set_writer(failing_writer, NULL) == ECOS_OK);
    assert(ecos_log_write(ECOS_LOG_INFO, "core", NULL, 0, "failure") ==
           ECOS_ERR_TIMEOUT);
}

static void test_log_truncation(void)
{
    capture_t capture = {{0}, 0u, 0u};
    char message[512];
    int result;

    memset(message, 'x', sizeof(message) - 1u);
    message[sizeof(message) - 1u] = '\0';
    assert(ecos_log_set_writer(capture_writer, &capture) == ECOS_OK);
    assert(ecos_log_set_level(ECOS_LOG_DEBUG) == ECOS_OK);
    result = ecos_log_write(ECOS_LOG_INFO, "core", NULL, 0, "%s", message);
    assert(result > 0);
    assert(capture.size < CONFIG_ECOS_LOG_BUFFER_SIZE);
    assert(capture.size >= 5u);
    assert(memcmp(capture.data + capture.size - 5u, "...\r\n", 5u) == 0);
}

int main(void)
{
    test_error_model();
    test_error_control_flow();
    test_log_output();
    test_log_truncation();
    test_panic_on_error();
    return 0;
}
