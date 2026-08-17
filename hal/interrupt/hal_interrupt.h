#ifndef __HAL_INTERRUPT_H__
#define __HAL_INTERRUPT_H__

#include <stdint.h>

typedef void (*hal_intr_handler_t)(void *arg);

/**
 * @brief 初始化机器外部中断入口和 PLIC 上下文
 *
 * @return int 0 表示成功，-1 表示失败
 */
int hal_intr_init(void);

/**
 * @brief 为指定 PLIC 中断源分配处理函数
 *
 * @param source PLIC 中断源编号
 * @param priority PLIC 中断优先级
 * @param handler 中断处理函数
 * @param arg 传递给中断处理函数的参数
 * @return int 0 表示成功，-1 表示失败
 */
int hal_intr_alloc(uint32_t source,
                   uint32_t priority,
                   hal_intr_handler_t handler,
                   void *arg);

/**
 * @brief 释放指定 PLIC 中断源
 *
 * @param source PLIC 中断源编号
 * @return int 0 表示成功，-1 表示失败
 */
int hal_intr_free(uint32_t source);

/**
 * @brief 使能指定 PLIC 中断源
 *
 * @param source PLIC 中断源编号
 * @return int 0 表示成功，-1 表示失败
 */
int hal_intr_enable(uint32_t source);

/**
 * @brief 禁止指定 PLIC 中断源
 *
 * @param source PLIC 中断源编号
 * @return int 0 表示成功，-1 表示失败
 */
int hal_intr_disable(uint32_t source);

/**
 * @brief 设置当前机器模式 PLIC 优先级阈值
 *
 * @param threshold PLIC 优先级阈值
 */
void hal_intr_set_threshold(uint32_t threshold);

/**
 * @brief 开启机器外部中断和机器全局中断
 */
void hal_intr_global_enable(void);

/**
 * @brief 关闭机器全局中断和机器外部中断
 */
void hal_intr_global_disable(void);

/**
 * @brief 处理公共中断层未接管的同步异常或其他机器陷阱
 *
 * @param mcause 机器模式陷阱原因
 * @param mepc 机器模式陷阱返回地址
 */
__attribute__((noreturn)) void hal_exception_handler(uint32_t mcause,
                                                     uint32_t mepc);

#endif
