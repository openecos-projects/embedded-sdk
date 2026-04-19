#include "sgp30.h"
#include <stdbool.h>
#include <stddef.h>
#include "hal_i2c.h"

extern void delay_ms(uint32_t val);

static uint8_t sensirion_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ 0x31);
            else crc = (uint8_t)(crc << 1);
        }
    }
    return crc;
}

// 发送命令到SGP30（无数据）
static bool sgp30_send_command(uint16_t command) {
    // SGP30命令不需要寄存器地址，直接发送命令字节
    // 使用reg_addr=0和HAL_I2C_REG_8，但实际上只发送data部分
    hal_i2c_write_nbyte(HAL_I2C_PORT_0, SGP30_I2C_ADDR, command, HAL_I2C_REG_16, NULL, 0);
    return true;
}

// 发送命令并读取响应
static bool sgp30_read_command(uint16_t command, uint8_t *data, uint8_t len) {
    if (!data || len == 0) return false;
    
    // 等待测量完成（根据命令不同等待时间不同）
    if (command == SGP30_CMD_MEASURE_AIR_QUALITY) {
        delay_ms(12);  // 空气质量测量需要12ms
    } else if (command == SGP30_CMD_MEASURE_TEST) {
        delay_ms(220); // 自测试需要220ms
    } else {
        delay_ms(10);  // 其他命令默认10ms
    }
    
    // 直接使用I2C读取，SGP30的读取命令就是发送命令字然后读取数据
    hal_i2c_read_nbyte(HAL_I2C_PORT_0, SGP30_I2C_ADDR, command, HAL_I2C_REG_16, data, len);
    return true;
}

bool sgp30_read_serial_id(uint64_t *serial_id) {
    if (!serial_id) return false;
    uint8_t rx[9] = {0};
    
    hal_i2c_read_nbyte(HAL_I2C_PORT_0, SGP30_I2C_ADDR, SGP30_CMD_GET_SERIAL_ID, HAL_I2C_REG_16, rx, sizeof(rx));

    // 校验每个数据字的CRC
    for (int i = 0; i < 9; i += 3) {
        if (sensirion_crc8(&rx[i], 2) != rx[i+2]) {
            return false;
        }
    }
    // 组装为48位序列号（大端序）
    uint64_t id = 0;
    id |= ((uint64_t)rx[0] << 40) | ((uint64_t)rx[1] << 32);
    id |= ((uint64_t)rx[3] << 24) | ((uint64_t)rx[4] << 16);
    id |= ((uint64_t)rx[6] << 8)  | ((uint64_t)rx[7] << 0);
    *serial_id = id;
    return true;
}

bool sgp30_read_feature_set(uint16_t *feature_set) {
    if (!feature_set) return false;
    uint8_t rx[3] = {0};
    
    hal_i2c_read_nbyte(HAL_I2C_PORT_0, SGP30_I2C_ADDR, SGP30_CMD_GET_FEATURE_SET, HAL_I2C_REG_16, rx, sizeof(rx));
    
    if (sensirion_crc8(rx, 2) != rx[2]) {
        return false;
    }
    *feature_set = ((uint16_t)rx[0] << 8) | rx[1];
    return true;
}

bool sgp30_init_air_quality(void) {
    return sgp30_send_command(SGP30_CMD_INIT_AIR_QUALITY);
}

bool sgp30_measure_air_quality(sgp30_air_quality_t *air_quality) {
    if (!air_quality) return false;
    
    uint8_t rx[6] = {0};
    if (!sgp30_read_command(SGP30_CMD_MEASURE_AIR_QUALITY, rx, sizeof(rx))) {
        return false;
    }
    
    // 校验两个数据字的CRC
    if (sensirion_crc8(&rx[0], 2) != rx[2] || 
        sensirion_crc8(&rx[3], 2) != rx[5]) {
        return false;
    }
    
    // 组装数据：CO2当量 (ppm) 和 TVOC (ppb)
    air_quality->co2_eq_ppm = ((uint16_t)rx[0] << 8) | rx[1];
    air_quality->tvoc_ppb = ((uint16_t)rx[3] << 8) | rx[4];
    
    return true;
}

bool sgp30_get_baseline(sgp30_baseline_t *baseline) {
    if (!baseline) return false;
    
    uint8_t rx[6] = {0};
    if (!sgp30_read_command(SGP30_CMD_GET_BASELINE, rx, sizeof(rx))) {
        return false;
    }
    
    // 校验两个数据字的CRC
    if (sensirion_crc8(&rx[0], 2) != rx[2] || 
        sensirion_crc8(&rx[3], 2) != rx[5]) {
        return false;
    }
    
    // 组装基线数据
    baseline->co2_eq_baseline = ((uint16_t)rx[0] << 8) | rx[1];
    baseline->tvoc_baseline = ((uint16_t)rx[3] << 8) | rx[4];
    
    return true;
}

bool sgp30_set_baseline(const sgp30_baseline_t *baseline) {
    if (!baseline) return false;
    
    uint8_t data[6];
    
    // TVOC基线 (先发送TVOC基线)
    data[0] = (uint8_t)(baseline->tvoc_baseline >> 8);
    data[1] = (uint8_t)(baseline->tvoc_baseline & 0xFF);
    data[2] = sensirion_crc8(&data[0], 2);
    
    // CO2当量基线
    data[3] = (uint8_t)(baseline->co2_eq_baseline >> 8);
    data[4] = (uint8_t)(baseline->co2_eq_baseline & 0xFF);
    data[5] = sensirion_crc8(&data[3], 2);
    
    // 使用I2C API发送设置基线命令和数据
    hal_i2c_write_nbyte(HAL_I2C_PORT_0, SGP30_I2C_ADDR, SGP30_CMD_SET_BASELINE, HAL_I2C_REG_16, data, 6);
    return true;
}

bool sgp30_set_humidity(uint16_t humidity) {
    uint8_t data[3];
    data[0] = (uint8_t)(humidity >> 8);
    data[1] = (uint8_t)(humidity & 0xFF);
    data[2] = sensirion_crc8(data, 2);
    
    // 使用I2C API发送设置湿度命令和数据
    hal_i2c_write_nbyte(HAL_I2C_PORT_0, SGP30_I2C_ADDR, SGP30_CMD_SET_HUMIDITY, HAL_I2C_REG_16, data, 3);
    return true;
}

bool sgp30_measure_test(uint16_t *test_result) {
    if (!test_result) return false;
    
    uint8_t rx[3] = {0};
    if (!sgp30_read_command(SGP30_CMD_MEASURE_TEST, rx, sizeof(rx))) {
        return false;
    }
    
    if (sensirion_crc8(rx, 2) != rx[2]) {
        return false;
    }
    
    *test_result = ((uint16_t)rx[0] << 8) | rx[1];
    return true;
}