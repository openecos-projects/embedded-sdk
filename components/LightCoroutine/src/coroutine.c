#include "coroutine.h"
#ifdef USE_CTX
#include "context.h"
#endif

static task_t avail_tasks[MAX_TASK_NUM+1];
static uint8_t resume_tasks[MAX_TASK_NUM+1];
static uint32_t resume_cnt;
static uint8_t suspend_tasks[MAX_TASK_NUM+1];
static uint32_t suspend_cnt;

task_t create_task(int id, func_t func, void* args){
    task_t task_handler = {
        .id = id,
        .func = func,
        .args = args,
        .resume_point = NULL,
    #ifdef USE_CTX
        .ctx = {0},
        // .ctx_top = TASK_CTX_SIZE - 1, // 指向栈顶元素的下一个
    #endif
    };
    avail_tasks[id] = task_handler;
    resume_tasks[id] = 1;
    suspend_tasks[id] = 0;
    resume_cnt++;
    return avail_tasks[id];
}

void delete_task(task_t* task_handler){
    if(task_handler->func==NULL){
        printf("error: task_handler is NULL\r\n");
        return;
    }
    avail_tasks[task_handler->id].func = NULL;
    if(resume_tasks[task_handler->id]) resume_cnt--;
    else suspend_cnt--;
    resume_tasks[task_handler->id] = 0;
    suspend_tasks[task_handler->id] = 0;
    task_handler->func = NULL;
}

void suspend_task(task_t* task_handler){
    if(resume_tasks[task_handler->id]){
        resume_tasks[task_handler->id] = 0;
        suspend_tasks[task_handler->id] = 1;
        resume_cnt--;
        suspend_cnt++;
    }
}

void resume_task(task_t* task_handler){
    if(suspend_tasks[task_handler->id]){
        suspend_tasks[task_handler->id] = 0;
        resume_tasks[task_handler->id] = 1;
        resume_cnt++;
        suspend_cnt--;
    }
}

static void idleHook(task_t* t){
    LOCAL(
        uint32_t unused[TASK_CTX_SIZE];
    );
    TASK_BEGIN(t);
    while(1){
        printf(">>>Enter Idle Task\r\n");
        printf(">>>Suspend Task Num: %d\r\n", suspend_cnt);
        printf(">>>Resume Task Num: %d\r\n", resume_cnt);
        printf(">>>Exit Idle Task\r\n\r\n");
        TASK_YIELD(t);  
    }
    TASK_END(t);
}

void task_scheduler(int round){

    uint32_t cnt = MAX_TASK_NUM;
#ifdef USE_IDLE_TASK
    task_t idle = create_task(MAX_TASK_NUM, idleHook, NULL);
    cnt++;
#endif

    if(round == -1){
        while(1){
            for(int i=0;i<cnt;i++){
                if(avail_tasks[i].func != NULL && (resume_tasks[i] || i==cnt-1 || (suspend_tasks[i] && EXEC_WAIT_POLICY))){
                    avail_tasks[i].func(&avail_tasks[i]);
                }
            }
        }
    }else{
        for(int i=0;i<round;i++){
            for(int j=0;j<cnt;j++){
                if(avail_tasks[j].func != NULL && (resume_tasks[j] || j==cnt-1 || (suspend_tasks[j] && EXEC_WAIT_POLICY))){
                    avail_tasks[j].func(&avail_tasks[j]);
                }
            }
        }
    }
    
}