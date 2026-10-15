#ifndef COROUTINE_H
#define COROUTINE_H
#include <stdint.h>
#include "conf.h"
#include "yield.h"



typedef struct task_t{
    int id;
    void* resume_point;
    void (*func)(struct task_t*);
    void *args;
#ifdef USE_CTX
    uint32_t ctx[TASK_CTX_SIZE]; // stk
#endif
} task_t;

typedef void (*func_t)(task_t*);

task_t create_task(int id, func_t func, void* args);
void delete_task(task_t* task_handler);
void suspend_task(task_t* task_handler);
void resume_task(task_t* task_handler);
void task_scheduler(int round);

#ifdef USE_CTX
/* Context helpers are part of the implementation used by yield.h macros. */
#include "context.h"
#endif

#endif
