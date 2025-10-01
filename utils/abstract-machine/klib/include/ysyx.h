#ifndef _YSYX_H__
#define _YSYX_H__

#include "stdint.h"
#include <stdint.h>

/****************************************************************************/
/*                              ARCHINFO                                    */
/****************************************************************************/
void ARCH_Info();
void ARCH_SetSys(uint32_t val);
void ARCH_SetIdl(uint32_t val);
void ARCH_SetIdh(uint32_t val);
#define ARCHINFO_BASE_ADDR 0x10017000
#define ARCHINFO_REG_SYS_OFFSET 0
#define ARCHINFO_REG_IDL_OFFSET 4
#define ARCHINFO_REG_IDH_OFFSET 8

#define ARCHINFO_REG_SYS   *((volatile uint32_t *)(ARCHINFO_BASE_ADDR + ARCHINFO_REG_SYS_OFFSET))
#define ARCHINFO_REG_IDL   *((volatile uint32_t *)(ARCHINFO_BASE_ADDR + ARCHINFO_REG_IDL_OFFSET))
#define ARCHINFO_REG_IDH   *((volatile uint32_t *)(ARCHINFO_BASE_ADDR + ARCHINFO_REG_IDH_OFFSET))

/****************************************************************************/
/*                              RNG                                         */
/****************************************************************************/

void RNG_SetCtrl(uint32_t val);
void RNG_SetSeed(uint32_t val);
uint32_t RNG_GetVal();

#define RNG_BASE_ADDR 0x10014000

#define RNG_REG_CTRL_OFFSET 0
#define RNG_REG_SEED_OFFSET 4
#define RNG_REG_VAL_OFFSET  8
#define RNG_REG_CTRL  *((volatile uint32_t *)(RNG_BASE_ADDR + RNG_REG_CTRL_OFFSET))
#define RNG_REG_SEED  *((volatile uint32_t *)(RNG_BASE_ADDR + RNG_REG_SEED_OFFSET))
#define RNG_REG_VAL   *((volatile uint32_t *)(RNG_BASE_ADDR + RNG_REG_VAL_OFFSET))

/****************************************************************************/
/*                              CRC                                         */
/****************************************************************************/

void CRC_SetCtrl(uint32_t val);
void CRC_SetInit(uint32_t val);
void CRC_SetXorV(uint32_t val);
void CRC_SetData(uint32_t val);
uint32_t CRC_GetVal();

#define CRC_BASE_ADDR 0x10015000

#define CRC_REG_CTRL_OFFSET 0
#define CRC_REG_INIT_OFFSET 4
#define CRC_REG_XORV_OFFSET 8
#define CRC_REG_DATA_OFFSET 12
#define CRC_REG_STAT_OFFSET 16

#define CRC_REG_CTRL  *((volatile uint32_t *)(CRC_BASE_ADDR + CRC_REG_CTRL_OFFSET))
#define CRC_REG_INIT  *((volatile uint32_t *)(CRC_BASE_ADDR + CRC_REG_INIT_OFFSET))
#define CRC_REG_XORV  *((volatile uint32_t *)(CRC_BASE_ADDR + CRC_REG_XORV_OFFSET))
#define CRC_REG_DATA  *((volatile uint32_t *)(CRC_BASE_ADDR + CRC_REG_DATA_OFFSET))
#define CRC_REG_STAT  *((volatile uint32_t *)(CRC_BASE_ADDR + CRC_REG_STAT_OFFSET))
/****************************************************************************/
/*                              GPIO                                        */
/****************************************************************************/
#define GPIO_0 (0)
#define GPIO_1 (1)
#define GPIO_IN  (0)
#define GPIO_OUT (1)

void GPIO_SetDir(uint32_t gpio, uint32_t dir);
void GPIO_SetDir_Num(uint32_t gpio, uint32_t pin, uint32_t dir);
void GPIO_SetVal(uint32_t gpio, uint32_t val);
void GPIO_SetVal_Num(uint32_t gpio, uint32_t pin, uint32_t val);
void GPIO_SetFCFG(uint32_t gpio, uint32_t val);
void GPIO_SetFCFG_Num(uint32_t gpio, uint32_t pin, uint32_t val);
void GPIO_SetMUX(uint32_t gpio, uint32_t val);
void GPIO_SetMUX_Num(uint32_t gpio, uint32_t pin, uint32_t val);
uint32_t GPIO_GetVal(uint32_t gpio);
uint32_t GPIO_GetVal_Num(uint32_t gpio, uint32_t pin);
/****************************************************************************/
/*                              GPIO_0                                      */
/****************************************************************************/
#define GPIO_0_BASE_ADDR           (uint32_t)0x10002000
#define GPIO_0_REG_PADDIR_OFFSET   0
#define GPIO_0_REG_PADIN_OFFSET    4
#define GPIO_0_REG_PADOUT_OFFSET   8
#define GPIO_0_REG_INTEN_OFFSET    12
#define GPIO_0_REG_INTTYPE0_OFFSET 16
#define GPIO_0_REG_INTTYPE1_OFFSET 20
#define GPIO_0_REG_INTSTAT_OFFSET  24
#define GPIO_0_REG_IOFCFG_OFFSET   28
#define GPIO_0_REG_PINMUX_OFFSET   32

#define GPIO_0_REG_PADDIR   *((volatile uint32_t *)(GPIO_0_BASE_ADDR + GPIO_0_REG_PADDIR_OFFSET))
#define GPIO_0_REG_PADIN    *((volatile uint32_t *)(GPIO_0_BASE_ADDR + GPIO_0_REG_PADIN_OFFSET))
#define GPIO_0_REG_PADOUT   *((volatile uint32_t *)(GPIO_0_BASE_ADDR + GPIO_0_REG_PADOUT_OFFSET))
#define GPIO_0_REG_INTEN    *((volatile uint32_t *)(GPIO_0_BASE_ADDR + GPIO_0_REG_INTEN_OFFSET))
#define GPIO_0_REG_INTTYPE0 *((volatile uint32_t *)(GPIO_0_BASE_ADDR + GPIO_0_REG_INTTYPE0_OFFSET))
#define GPIO_0_REG_INTTYPE1 *((volatile uint32_t *)(GPIO_0_BASE_ADDR + GPIO_0_REG_INTTYPE1_OFFSET))
#define GPIO_0_REG_INTSTAT  *((volatile uint32_t *)(GPIO_0_BASE_ADDR + GPIO_0_REG_INTSTAT_OFFSET))
#define GPIO_0_REG_IOFCFG   *((volatile uint32_t *)(GPIO_0_BASE_ADDR + GPIO_0_REG_IOFCFG_OFFSET))
#define GPIO_0_REG_PINMUX   *((volatile uint32_t *)(GPIO_0_BASE_ADDR + GPIO_0_REG_PINMUX_OFFSET))


/****************************************************************************/
/*                              GPIO_1                                      */
/****************************************************************************/
#define GPIO_1_BASE_ADDR           (uint32_t)0x10003000
#define GPIO_1_REG_PADDIR_OFFSET   0
#define GPIO_1_REG_PADIN_OFFSET    4
#define GPIO_1_REG_PADOUT_OFFSET   8
#define GPIO_1_REG_INTEN_OFFSET    12
#define GPIO_1_REG_INTTYPE0_OFFSET 16
#define GPIO_1_REG_INTTYPE1_OFFSET 20
#define GPIO_1_REG_INTSTAT_OFFSET  24
#define GPIO_1_REG_IOFCFG_OFFSET   28
#define GPIO_1_REG_PINMUX_OFFSET   32

#define GPIO_1_REG_PADDIR   *((volatile uint32_t *)(GPIO_1_BASE_ADDR + GPIO_1_REG_PADDIR_OFFSET))
#define GPIO_1_REG_PADIN    *((volatile uint32_t *)(GPIO_1_BASE_ADDR + GPIO_1_REG_PADIN_OFFSET))
#define GPIO_1_REG_PADOUT   *((volatile uint32_t *)(GPIO_1_BASE_ADDR + GPIO_1_REG_PADOUT_OFFSET))
#define GPIO_1_REG_INTEN    *((volatile uint32_t *)(GPIO_1_BASE_ADDR + GPIO_1_REG_INTEN_OFFSET))
#define GPIO_1_REG_INTTYPE0 *((volatile uint32_t *)(GPIO_1_BASE_ADDR + GPIO_1_REG_INTTYPE0_OFFSET))
#define GPIO_1_REG_INTTYPE1 *((volatile uint32_t *)(GPIO_1_BASE_ADDR + GPIO_1_REG_INTTYPE1_OFFSET))
#define GPIO_1_REG_INTSTAT  *((volatile uint32_t *)(GPIO_1_BASE_ADDR + GPIO_1_REG_INTSTAT_OFFSET))
#define GPIO_1_REG_IOFCFG   *((volatile uint32_t *)(GPIO_1_BASE_ADDR + GPIO_1_REG_IOFCFG_OFFSET))
#define GPIO_1_REG_PINMUX   *((volatile uint32_t *)(GPIO_1_BASE_ADDR + GPIO_1_REG_PINMUX_OFFSET))


/****************************************************************************/
/*                                 WDG                                      */
/****************************************************************************/
void WDG_SetCtrl(uint32_t val);
void WDG_SetPrescale(uint32_t val);
void WDG_SetCmp(uint32_t val);
void WDG_Feed(uint32_t val);
void WDG_SetKey(uint32_t val);
uint32_t WDG_GetStat();

#define WDG_BASE_ADDR       (uint32_t)0x10008000
#define WDG_MAGIC_NUM       (uint32_t)0x5F3759DF
#define WDG_REG_CTRL_OFFSET 0
#define WDG_REG_PSCR_OFFSET 4
#define WDG_REG_CNT_OFFSET  8
#define WDG_REG_CMP_OFFSET  12
#define WDG_REG_STAT_OFFSET 16
#define WDG_REG_KEY_OFFSET  20
#define WDG_REG_FEED_OFFSET 24

#define WDG_REG_CTRL *((volatile uint32_t *)(WDG_BASE_ADDR + WDG_REG_CTRL_OFFSET))
#define WDG_REG_PSCR *((volatile uint32_t *)(WDG_BASE_ADDR + WDG_REG_PSCR_OFFSET))
#define WDG_REG_CNT  *((volatile uint32_t *)(WDG_BASE_ADDR + WDG_REG_CNT_OFFSET))
#define WDG_REG_CMP  *((volatile uint32_t *)(WDG_BASE_ADDR + WDG_REG_CMP_OFFSET))
#define WDG_REG_STAT *((volatile uint32_t *)(WDG_BASE_ADDR + WDG_REG_STAT_OFFSET))
#define WDG_REG_KEY  *((volatile uint32_t *)(WDG_BASE_ADDR + WDG_REG_KEY_OFFSET))
#define WDG_REG_FEED *((volatile uint32_t *)(WDG_BASE_ADDR + WDG_REG_FEED_OFFSET))


/****************************************************************************/
/*                                 TIMER_0                                  */
/****************************************************************************/

void timer_init(uint32_t div, uint32_t cmp);
void delay_ms(uint32_t val);

#define TIMER_0_BASE_ADDR       (uint32_t)0x10009000
#define TIMER_0_REG_CTRL_OFFSET 0
#define TIMER_0_REG_PSCR_OFFSET 4
#define TIMER_0_REG_CNT_OFFSET  8
#define TIMER_0_REG_CMP_OFFSET  12
#define TIMER_0_REG_STAT_OFFSET 16

#define TIMER_0_REG_CTRL *((volatile uint32_t *)(TIMER_0_BASE_ADDR + TIMER_0_REG_CTRL_OFFSET))
#define TIMER_0_REG_PSCR *((volatile uint32_t *)(TIMER_0_BASE_ADDR + TIMER_0_REG_PSCR_OFFSET))
#define TIMER_0_REG_CNT  *((volatile uint32_t *)(TIMER_0_BASE_ADDR + TIMER_0_REG_CNT_OFFSET))
#define TIMER_0_REG_CMP  *((volatile uint32_t *)(TIMER_0_BASE_ADDR + TIMER_0_REG_CMP_OFFSET))
#define TIMER_0_REG_STAT *((volatile uint32_t *)(TIMER_0_BASE_ADDR + TIMER_0_REG_STAT_OFFSET))

/****************************************************************************/
/*                                  PWM                                     */
/****************************************************************************/
#define PWM_0 (0)
#define PWM_1 (1)
#define PWM_2 (2)

#define PWM_CHANAL_0 (0)
#define PWM_CHANAL_1 (1)
#define PWM_CHANAL_2 (2)
#define PWM_CHANAL_3 (3)

void PWM_Init(uint32_t pwm, uint32_t prescale, uint32_t cmp);
void PWM_SetCtr(uint32_t pwm, uint32_t val);
void PWM_SetPrescale(uint32_t pwm, uint32_t val);
void PWM_SetCmp(uint32_t pwm, uint32_t val);
void PWM_SetDuty(uint32_t pwm, uint32_t channel, uint32_t duty);

/****************************************************************************/
/*                                  PWM_0                                   */
/****************************************************************************/



#define PWM_0_BASE_ADDR       (uint32_t)0x10004000
#define PWM_0_REG_CTRL_OFFSET 0
#define PWM_0_REG_PSCR_OFFSET 4
#define PWM_0_REG_CNT_OFFSET  8
#define PWM_0_REG_CMP_OFFSET  12
#define PWM_0_REG_CR0_OFFSET  16
#define PWM_0_REG_CR1_OFFSET  20
#define PWM_0_REG_CR2_OFFSET  24
#define PWM_0_REG_CR3_OFFSET  28
#define PWM_0_REG_STAT_OFFSET 32

#define PWM_0_REG_CTRL *((volatile uint32_t *)(PWM_0_BASE_ADDR + PWM_0_REG_CTRL_OFFSET))
#define PWM_0_REG_PSCR *((volatile uint32_t *)(PWM_0_BASE_ADDR + PWM_0_REG_PSCR_OFFSET))
#define PWM_0_REG_CNT  *((volatile uint32_t *)(PWM_0_BASE_ADDR + PWM_0_REG_CNT_OFFSET))
#define PWM_0_REG_CMP  *((volatile uint32_t *)(PWM_0_BASE_ADDR + PWM_0_REG_CMP_OFFSET))
#define PWM_0_REG_CR0  *((volatile uint32_t *)(PWM_0_BASE_ADDR + PWM_0_REG_CR0_OFFSET))
#define PWM_0_REG_CR1  *((volatile uint32_t *)(PWM_0_BASE_ADDR + PWM_0_REG_CR1_OFFSET))
#define PWM_0_REG_CR2  *((volatile uint32_t *)(PWM_0_BASE_ADDR + PWM_0_REG_CR2_OFFSET))
#define PWM_0_REG_CR3  *((volatile uint32_t *)(PWM_0_BASE_ADDR + PWM_0_REG_CR3_OFFSET))
#define PWM_0_REG_STAT *((volatile uint32_t *)(PWM_0_BASE_ADDR + PWM_0_REG_STAT_OFFSET))


/****************************************************************************/
/*                                  PWM_1                                   */
/****************************************************************************/
#define PWM_1_BASE_ADDR       (uint32_t)0x10005000
#define PWM_1_REG_CTRL_OFFSET 0
#define PWM_1_REG_PSCR_OFFSET 4
#define PWM_1_REG_CNT_OFFSET  8
#define PWM_1_REG_CMP_OFFSET  12
#define PWM_1_REG_CR0_OFFSET  16
#define PWM_1_REG_CR1_OFFSET  20
#define PWM_1_REG_CR2_OFFSET  24
#define PWM_1_REG_CR3_OFFSET  28
#define PWM_1_REG_STAT_OFFSET 32

#define PWM_1_REG_CTRL *((volatile uint32_t *)(PWM_1_BASE_ADDR + PWM_1_REG_CTRL_OFFSET))
#define PWM_1_REG_PSCR *((volatile uint32_t *)(PWM_1_BASE_ADDR + PWM_1_REG_PSCR_OFFSET))
#define PWM_1_REG_CNT  *((volatile uint32_t *)(PWM_1_BASE_ADDR + PWM_1_REG_CNT_OFFSET))
#define PWM_1_REG_CMP  *((volatile uint32_t *)(PWM_1_BASE_ADDR + PWM_1_REG_CMP_OFFSET))
#define PWM_1_REG_CR0  *((volatile uint32_t *)(PWM_1_BASE_ADDR + PWM_1_REG_CR0_OFFSET))
#define PWM_1_REG_CR1  *((volatile uint32_t *)(PWM_1_BASE_ADDR + PWM_1_REG_CR1_OFFSET))
#define PWM_1_REG_CR2  *((volatile uint32_t *)(PWM_1_BASE_ADDR + PWM_1_REG_CR2_OFFSET))
#define PWM_1_REG_CR3  *((volatile uint32_t *)(PWM_1_BASE_ADDR + PWM_1_REG_CR3_OFFSET))
#define PWM_1_REG_STAT *((volatile uint32_t *)(PWM_1_BASE_ADDR + PWM_1_REG_STAT_OFFSET))


/****************************************************************************/
/*                                  PWM_2                                   */
/****************************************************************************/
#define PWM_2_BASE_ADDR       (uint32_t)0x10006000
#define PWM_2_REG_CTRL_OFFSET 0
#define PWM_2_REG_PSCR_OFFSET 4
#define PWM_2_REG_CNT_OFFSET  8
#define PWM_2_REG_CMP_OFFSET  12
#define PWM_2_REG_CR0_OFFSET  16
#define PWM_2_REG_CR1_OFFSET  20
#define PWM_2_REG_CR2_OFFSET  24
#define PWM_2_REG_CR3_OFFSET  28
#define PWM_2_REG_STAT_OFFSET 32

#define PWM_2_REG_CTRL *((volatile uint32_t *)(PWM_2_BASE_ADDR + PWM_2_REG_CTRL_OFFSET))
#define PWM_2_REG_PSCR *((volatile uint32_t *)(PWM_2_BASE_ADDR + PWM_2_REG_PSCR_OFFSET))
#define PWM_2_REG_CNT  *((volatile uint32_t *)(PWM_2_BASE_ADDR + PWM_2_REG_CNT_OFFSET))
#define PWM_2_REG_CMP  *((volatile uint32_t *)(PWM_2_BASE_ADDR + PWM_2_REG_CMP_OFFSET))
#define PWM_2_REG_CR0  *((volatile uint32_t *)(PWM_2_BASE_ADDR + PWM_2_REG_CR0_OFFSET))
#define PWM_2_REG_CR1  *((volatile uint32_t *)(PWM_2_BASE_ADDR + PWM_2_REG_CR1_OFFSET))
#define PWM_2_REG_CR2  *((volatile uint32_t *)(PWM_2_BASE_ADDR + PWM_2_REG_CR2_OFFSET))
#define PWM_2_REG_CR3  *((volatile uint32_t *)(PWM_2_BASE_ADDR + PWM_2_REG_CR3_OFFSET))
#define PWM_2_REG_STAT *((volatile uint32_t *)(PWM_2_BASE_ADDR + PWM_2_REG_STAT_OFFSET))


/****************************************************************************/
/*                                   I2C                                    */
/****************************************************************************/
#define I2C_BASE_ADDR        (uint32_t)0x1000D000
#define I2C_REG_CTRL_OFFSET  0
#define I2C_REG_PSCRL_OFFSET 4
#define I2C_REG_PSCRH_OFFSET 5
#define I2C_REG_TXR_OFFSET   8
#define I2C_REG_RXR_OFFSET   12
#define I2C_REG_CMD_OFFSET   16
#define I2C_REG_SR_OFFSET    20

#define I2C_REG_CTRL  *((volatile uint8_t *)(I2C_BASE_ADDR + I2C_REG_CTRL_OFFSET))
#define I2C_REG_PSCRL *((volatile uint8_t *)(I2C_BASE_ADDR + I2C_REG_PSCRL_OFFSET))
#define I2C_REG_PSCRH *((volatile uint8_t *)(I2C_BASE_ADDR + I2C_REG_PSCRH_OFFSET))
#define I2C_REG_TXR   *((volatile uint8_t *)(I2C_BASE_ADDR + I2C_REG_TXR_OFFSET))
#define I2C_REG_RXR   *((volatile uint8_t *)(I2C_BASE_ADDR + I2C_REG_RXR_OFFSET))
#define I2C_REG_CMD   *((volatile uint8_t *)(I2C_BASE_ADDR + I2C_REG_CMD_OFFSET))
#define I2C_REG_SR    *((volatile uint8_t *)(I2C_BASE_ADDR + I2C_REG_SR_OFFSET))


/****************************************************************************/
/*                                   RCU                                    */
/****************************************************************************/

void RCU_SetCtrl(uint32_t val);
void RCU_SetRdiv(uint32_t val);
uint32_t RCU_GetStat();

#define RCU_BASE_ADDR       (uint32_t)0x10018000
#define RCU_REG_CTRL_OFFSET 0
#define RCU_REG_RDIV_OFFSET 4
#define RCU_REG_STAT_OFFSET 8

#define RCU_REG_CTRL  *((volatile uint32_t *)(RCU_BASE_ADDR + RCU_REG_CTRL_OFFSET))
#define RCU_REG_RDIV  *((volatile uint32_t *)(RCU_BASE_ADDR + RCU_REG_RDIV_OFFSET))
#define RCU_REG_STAT  *((volatile uint32_t *)(RCU_BASE_ADDR + RCU_REG_STAT_OFFSET))


/****************************************************************************/
/*                                   RTC                                    */
/****************************************************************************/
void RTC_SetCtrl(uint32_t val);
void RTC_SetPrescale(uint32_t val);
void RTC_SetCnt(uint32_t val);
uint32_t RTC_GetCnt();
void RTC_SetAlrm(uint32_t val);
uint32_t RTC_GetIsta();

#define RTC_BASE_ADDR       (uint32_t)0x10007000
#define RTC_REG_CTRL_OFFSET 0
#define RTC_REG_PSCR_OFFSET 4
#define RTC_REG_CNT_OFFSET  8
#define RTC_REG_ALRM_OFFSET 12
#define RTC_REG_ISTA_OFFSET 16
#define RTC_REG_SSTA_OFFSET 20

#define RTC_REG_CTRL  *((volatile uint32_t *)(RTC_BASE_ADDR + RTC_REG_CTRL_OFFSET))
#define RTC_REG_PSCR  *((volatile uint32_t *)(RTC_BASE_ADDR + RTC_REG_PSCR_OFFSET))
#define RTC_REG_CNT   *((volatile uint32_t *)(RTC_BASE_ADDR + RTC_REG_CNT_OFFSET))
#define RTC_REG_ALRM  *((volatile uint32_t *)(RTC_BASE_ADDR + RTC_REG_ALRM_OFFSET))
#define RTC_REG_ISTA  *((volatile uint32_t *)(RTC_BASE_ADDR + RTC_REG_ISTA_OFFSET))
#define RTC_REG_SSTA  *((volatile uint32_t *)(RTC_BASE_ADDR + RTC_REG_SSTA_OFFSET))


/****************************************************************************/
/*                                   CLINT                                  */
/****************************************************************************/
#define CLINT_BASE_ADDR            (uint32_t)0x01FF0000
#define CLINT_REG_MSIP_OFFSET      0
#define CLINT_REG_MTIMEL_OFFSET    4
#define CLINT_REG_MTIMEH_OFFSET    8
#define CLINT_REG_MTIMECMPL_OFFSET 12
#define CLINT_REG_MTIMECMPH_OFFSET 16

#define CLINT_REG_MSIP      *((volatile uint32_t *)(CLINT_BASE_ADDR + CLINT_REG_MSIP_OFFSET))
#define CLINT_REG_MTIMEL    *((volatile uint32_t *)(CLINT_BASE_ADDR + CLINT_REG_MTIMEL_OFFSET))
#define CLINT_REG_MTIMEH    *((volatile uint32_t *)(CLINT_BASE_ADDR + CLINT_REG_MTIMEH_OFFSET))
#define CLINT_REG_MTIMECMPL *((volatile uint32_t *)(CLINT_BASE_ADDR + CLINT_REG_MTIMECMPL_OFFSET))
#define CLINT_REG_MTIMECMPH *((volatile uint32_t *)(CLINT_BASE_ADDR + CLINT_REG_MTIMECMPH_OFFSET))


/****************************************************************************/
/*                                   PS2                                    */
/****************************************************************************/
void PS2_SetCtrl(uint32_t val);
uint32_t PS2_GetData();
#define PS2_BASE_ADDR       (uint32_t)0x10013000 // GPIO1 FUNC0 12, 13
#define PS2_REG_CTRL_OFFSET 0
#define PS2_REG_DATA_OFFSET 4
#define PS2_REG_STAT_OFFSET 8

#define PS2_REG_CTRL  *((volatile uint32_t *)(PS2_BASE_ADDR + PS2_REG_CTRL_OFFSET))
#define PS2_REG_DATA  *((volatile uint32_t *)(PS2_BASE_ADDR + PS2_REG_DATA_OFFSET))
#define PS2_REG_STAT  *((volatile uint32_t *)(PS2_BASE_ADDR + PS2_REG_STAT_OFFSET))


/****************************************************************************/
/*                                   SPI1                                   */
/****************************************************************************/
void SPI1_Init();
void spi1_wr_dat(uint8_t dat);
void spi1_wr_dat16(uint16_t dat);

#define SPI1_BASE_ADDR         (uint32_t)0x10010000
#define SPI1_REG_STATUS_OFFSET 0
#define SPI1_REG_CLKDIV_OFFSET 4
#define SPI1_REG_CMD_OFFSET    8
#define SPI1_REG_ADR_OFFSET    12
#define SPI1_REG_LEN_OFFSET    16
#define SPI1_REG_DUM_OFFSET    20
#define SPI1_REG_TXFIFO_OFFSET 24
#define SPI1_REG_RXFIFO_OFFSET 32
#define SPI1_REG_INTCFG_OFFSET 36
#define SPI1_REG_INTSTA_OFFSET 40

#define SPI1_REG_STATUS  *((volatile uint32_t *)(SPI1_BASE_ADDR + SPI1_REG_STATUS_OFFSET))
#define SPI1_REG_CLKDIV  *((volatile uint32_t *)(SPI1_BASE_ADDR + SPI1_REG_CLKDIV_OFFSET))
#define SPI1_REG_CMD     *((volatile uint32_t *)(SPI1_BASE_ADDR + SPI1_REG_CMD_OFFSET))
#define SPI1_REG_ADR     *((volatile uint32_t *)(SPI1_BASE_ADDR + SPI1_REG_ADR_OFFSET))
#define SPI1_REG_LEN     *((volatile uint32_t *)(SPI1_BASE_ADDR + SPI1_REG_LEN_OFFSET))
#define SPI1_REG_DUM     *((volatile uint32_t *)(SPI1_BASE_ADDR + SPI1_REG_DUM_OFFSET))
#define SPI1_REG_TXFIFO  *((volatile uint32_t *)(SPI1_BASE_ADDR + SPI1_REG_TXFIFO_OFFSET))
#define SPI1_REG_RXFIFO  *((volatile uint32_t *)(SPI1_BASE_ADDR + SPI1_REG_RXFIFO_OFFSET))
#define SPI1_REG_INTCFG  *((volatile uint32_t *)(SPI1_BASE_ADDR + SPI1_REG_INTCFG_OFFSET))
#define SPI1_REG_INTSTA  *((volatile uint32_t *)(SPI1_BASE_ADDR + SPI1_REG_INTSTA_OFFSET))


/****************************************************************************/
/*                                   Drive                                  */
/****************************************************************************/

// System
void info_id();

// SPI_TFT_LCD
#define LCD_W 128
#define LCD_H 128
#define lcd_dc_clr (GPIO_1_REG_PADOUT = GPIO_1_REG_PADOUT & ~(0x01 << 6))
#define lcd_dc_set (GPIO_1_REG_PADOUT = GPIO_1_REG_PADOUT | (0x01 << 6))
#define USE_HORIZONTAL 0 // 0:竖屏 1:横屏 2:竖屏翻转180度 3:横屏翻转180度
void spi_tft_init();
void lcd_wr_cmd(uint8_t cmd);
void lcd_wr_data8(uint8_t dat);
void lcd_wr_data16(uint16_t dat);
void lcd_addr_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend,uint32_t color);
void lcd_fill_bmp(const uint8_t *bmp, uint16_t x, uint16_t y, uint16_t w,uint16_t h);
void lcd_refresh(uint16_t *gdm, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

// I2C
#define I2C_TEST_START       ((uint8_t)0x80)
#define I2C_TEST_STOP        ((uint8_t)0x40)
#define I2C_TEST_READ        ((uint8_t)0x20)
#define I2C_TEST_WRITE       ((uint8_t)0x10)
#define I2C_TEST_START_READ  ((uint8_t)0xA0)
#define I2C_TEST_START_WRITE ((uint8_t)0x90)
#define I2C_TEST_STOP_READ   ((uint8_t)0x60)
#define I2C_TEST_STOP_WRITE  ((uint8_t)0x50)

#define I2C_STATUS_RXACK     ((uint8_t)0x80) // (1 << 7)
#define I2C_STATUS_BUSY      ((uint8_t)0x40) // (1 << 6)
#define I2C_STATUS_AL        ((uint8_t)0x20) // (1 << 5)
#define I2C_STATUS_TIP       ((uint8_t)0x02) // (1 << 1)
#define I2C_STATUS_IF        ((uint8_t)0x01) // (1 << 0)

#define I2C_DEV_ADDR_16BIT 0
#define I2C_DEV_ADDR_8BIT  1

void i2c_config();
void i2c_wr_start(uint32_t slv_addr);
void i2c_rd_start(uint32_t slv_addr);
void i2c_write(uint8_t val);
void i2c_stop();
void i2c_wr_nbyte(uint8_t slv_addr, uint16_t reg_addr, uint8_t type,uint8_t num, uint8_t *data);
void i2c_rd_nbyte(uint8_t slv_addr, uint16_t reg_addr, uint8_t type,uint8_t num, uint8_t *data);
uint32_t i2c_read(uint32_t cmd);
uint32_t i2c_get_ack();
uint32_t i2c_busy();

#endif
