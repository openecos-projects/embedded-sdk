# mini-os

一个建立在 SDK 现有组件之上的**协作式小型内核**示例。它把 LightCoroutine
的协程调度、Letter-Shell 的命令行和 Timer 驱动的 1 ms 时基组合成一个可以
交互的 mini OS，不引入 FreeRTOS 等第三方 RTOS。

## 组成

| 层 | 实现 | 说明 |
| --- | --- | --- |
| 任务与上下文 | `components/LightCoroutine` | 半有栈协程：`TASK_BEGIN` / `TASK_YIELD` / `TASK_WAIT`，任务局部变量保存在各自的 64 word 上下文中 |
| 任务表 / 调度 | `kernel.c` | 8 个静态 PCB、轮转调度、sleep/wake、suspend/resume、kill、tick 与 CPU 时间统计 |
| 时基 | `driver-timer` | Timer0 周期 1 ms；每次调度轮询到期标志并重启，到期失败时退化为“每轮 1 tick” |
| 交互 | `letter-shell` | shell 本身也是一个任务，用 `bsp_console_try_read()` 非阻塞轮询串口 |
| 命令 | `os_shell.c` | `ps` / `uptime` / `tick` / `spawn` / `kill` / `suspend` / `resume`，与 Letter-Shell 内置命令共存 |

镜像里没有动态内存：任务表、shell 缓冲区和命令表都是静态存储。

## 创建工程并构建

```sh
eval "$(ecos env)"

ecos project create mini-os -name my_os -target starrysky-l4-c1
cd my_os
ecos configure
ecos build
ecos flash --port /dev/ttyUSB0      # 按实际串口调整
ecos monitor --port /dev/ttyUSB0
```

`mini-os` 需要 `console` 和 `timer` 能力，当前除 StarrySky L3 外其余板卡都支持
（`ecos project list` 会显示兼容板卡）。

## 运行现象

上电后串口先输出启动日志，随后：

- `worker` 任务每 200 tick 打印一步，5 步后自行退出并被内核回收；
- `heartbeat` 任务每 1000 tick 打印一次 uptime；
- shell 提示符可随时输入命令。

```
mini-os: tick source: timer0, 1000 us
mini-os: [worker] step 0
mini-os: [heartbeat] beat=1 uptime=1000 ms
...
mini-os$ help
mini-os$ ps
id name       state  ticks switches wake
0 shell       ready  2     41       0
1 heartbeat   block  1     5        2000
2 worker      ready  1     6        1200
mini-os$ spawn
task 3 (worker) created
mini-os$ suspend 3
task 3 suspended
mini-os$ resume 3
task 3 resumed
mini-os$ kill 3
task 3 killed
```

`ps` 列含义：

- `state`：`ready` / `run` / `block`（等待 sleep 到期）/ `susp`（被挂起）；
- `ticks`：任务体累计占用的 tick 数；
- `switches`：被调度器进入的次数；
- `wake`：sleep 的唤醒 tick。

## 任务编写方式

任务函数必须使用 LightCoroutine 的协程语法，并且在 `TASK_BEGIN` 前声明
`LOCAL(...)`：

```c
static void my_task(task_t *task)
{
    LOCAL(uint32_t n;);

    TASK_BEGIN(task);
    for (;;) {
        ECOS_LOGI("mini-os", "hello %lu", (unsigned long)++local.n);
        OS_SLEEP(task, 500);   /* 阻塞 500 tick，让出 CPU */
    }
    TASK_END(task);
}

/* 在 shell 命令或 main() 中创建 */
os_task_create("my-task", my_task, NULL);
```

`OS_SLEEP()` 只是 `os_sleep_begin()` + `TASK_WAIT()` 的组合；其他等待条件可以
直接使用 `TASK_WAIT(task, condition)`。

## 设计边界

- **协作式，不抢占**：任务必须主动 `TASK_YIELD` / `OS_SLEEP` / `TASK_WAIT`，
  长循环会拖慢其他任务。这也意味着不需要在中断里做上下文切换。
- **tick 是轮询的**：`os_tick_update()` 每个调度点检查 Timer0 到期标志，
  每次最多补 4 个 tick；长时间占用 CPU 的任务会造成 tick 丢失。
- **单核、无锁**：中断上下文不访问任务表；串口读写都在任务上下文完成。
- **固定任务数**：`OS_MAX_TASKS` 默认为 8，按需修改 `kernel.h`，同时要保证
  不超过 LightCoroutine 的 `MAX_TASK_NUM`。
- 这是教学/原型级内核，不提供内存保护、系统调用、虚拟内存或优先级调度。
