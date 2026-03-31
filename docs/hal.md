# HAL接口设计
## 系统抽象层
### sys_uart
```c
/**
 * @brief 初始化系统串口
 * 
 */
void hal_sys_uart_init(void);

/**
 * @brief 发送一个字符到系统串口
 * 
 * @param c 要发送的字符
 */
void hal_sys_putchar(char c);

/**
 * @brief 发送一个字符串到系统串口
 * 
 * @param str 要发送的字符串
 */
void hal_sys_putstr(char *str);
```
## 外设抽象层
### gpio
```c
/**
 * @brief 启用指定GPIO为输入模式
 * 
 * @param hal GPIO HAL实例指针[暂时未使用]
 * @param gpio_num GPIO编号
 */
void gpio_hal_input_enable(void *hal, uint8_t gpio_num);

/**
 * @brief 启用指定GPIO为输出模式
 * 
 * @param hal GPIO HAL实例指针[暂时未使用]
 * @param gpio_num GPIO编号
 */
void gpio_hal_output_enable(void *hal, uint8_t gpio_num);

/**
 * @brief 设置指定GPIO为高电平
 * 
 * @param hal GPIO HAL实例指针[暂时未使用]
 * @param gpio_num GPIO编号
 */
void gpio_hal_set_level(void *hal, uint8_t gpio_num, uint8_t level);

/**
 * @brief 获取指定GPIO的电平
 * 
 * @param hal GPIO HAL实例指针[暂时未使用]
 * @param gpio_num GPIO编号
 * @return uint8_t GPIO电平
 */
uint8_t gpio_hal_get_level(void *hal, uint8_t gpio_num);
```
