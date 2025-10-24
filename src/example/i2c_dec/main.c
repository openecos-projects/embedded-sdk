#include "main.h"

static bool i2c_probe_device_quiet(uint8_t addr) {
    // 保存原来的stdout，临时重定向以抑制错误输出
    // 由于这是嵌入式环境，我们直接通过寄存器操作来检测ACK
    
    // 发送START + 写地址
    REG_CUST_I2C_TXR = (addr << 1) | 0; // 写操作
    REG_CUST_I2C_CMD = I2C_START_WRITE;
    
    // 等待传输完成
    while((REG_CUST_I2C_SR & I2C_STATUS_TIP) == 0);
    while((REG_CUST_I2C_SR & I2C_STATUS_TIP) != 0);
    
    // 检查ACK
    bool device_found = !(REG_CUST_I2C_SR & I2C_STATUS_RXACK);
    
    // 发送STOP
    REG_CUST_I2C_CMD = I2C_STOP;
    while((REG_CUST_I2C_SR & I2C_STATUS_BUSY) == I2C_STATUS_BUSY);
    
    return device_found;
}

bool i2c_probe_device(uint8_t addr) {
    return i2c_probe_device_quiet(addr);
}

uint8_t i2c_scan_devices(uint8_t start_addr, uint8_t end_addr, uint8_t* found_devices, uint8_t max_devices) {
    uint8_t device_count = 0;
    
    if (!found_devices || max_devices == 0) {
        return 0;
    }
    
    for (uint8_t addr = start_addr; addr <= end_addr && device_count < max_devices; addr++) {
        if (i2c_probe_device_quiet(addr)) {
            found_devices[device_count] = addr;
            device_count++;
        }
    }
    
    return device_count;
}

void i2c_print_scan_result(uint8_t start_addr, uint8_t end_addr) {
    printf("\nI2C Device Scanner\n");
    printf("==================\n");
    printf("Scanning I2C addresses from 0x%X to 0x%X...\n\n", start_addr, end_addr);
    
    // 打印表头
    printf("    ");
    for (uint8_t col = 0; col < 16; col++) {
        printf("  %X", col);
    }
    printf("\n");
    
    // 打印扫描结果表格
    for (uint8_t row = 0; row < 8; row++) {
        printf("%X: ", row * 16);
        
        for (uint8_t col = 0; col < 16; col++) {
            uint8_t addr = row * 16 + col;
            
            if (addr < start_addr || addr > end_addr) {
                printf("   ");  // 超出扫描范围
            } else if (addr < 0x08 || addr > 0x77) {
                printf(" --");  // 保留地址
            } else if (i2c_probe_device_quiet(addr)) {
                printf(" %X", addr);  // 找到设备
            } else {
                printf(" ..");  // 无设备响应
            }
        }
        printf("\n");
    }
    
    printf("\nLegend:\n");
    printf("  XX = Device found at address 0xXX\n");
    printf("  .. = No device found\n");
    printf("  -- = Reserved address\n\n");
    
    // 统计并列出找到的设备
    uint8_t found_devices[120];  // 最多120个设备地址
    uint8_t device_count = i2c_scan_devices(start_addr, end_addr, found_devices, sizeof(found_devices));
    
    if (device_count > 0) {
        printf("Found %d device(s):\n", device_count);
        for (uint8_t i = 0; i < device_count; i++) {
            printf("  - 0x%X", found_devices[i]);
            
            // 添加一些常见设备的识别
            switch (found_devices[i]) {
                case 0x3C:
                case 0x3D:
                    printf(" (OLED Display SSD1306)");
                    break;
                case 0x48:
                    printf(" (ADS1115 ADC or TMP102 Temperature)");
                    break;
                case 0x50:
                case 0x51:
                case 0x52:
                case 0x53:
                case 0x54:
                case 0x55:
                case 0x56:
                case 0x57:
                    printf(" (EEPROM 24CXX)");
                    break;
                case 0x58:
                    printf(" (SGP30 Air Quality)");
                    break;
                case 0x68:
                    printf(" (DS1307 RTC or MPU6050 IMU)");
                    break;
                case 0x76:
                case 0x77:
                    printf(" (BMP280/BME280 Pressure)");
                    break;
                default:
                    printf(" (Unknown device)");
                    break;
            }
            printf("\n");
        }
    } else {
        printf("No I2C devices found.\n");
    }
    printf("\n");
}

void main(void){
    
    sys_uart_init();
    printf("Hello, World!\n");

    // 初始化I2C
    i2c_config_t i2c_config = {
        .pscr = 99,
    };
    i2c_init(&i2c_config);

    // 执行I2C设备扫描
    printf("\n=== I2C Device Scanner ===\n");
    i2c_print_scan_result(0x08, 0x77);

    // 演示单个设备检测
    printf("=== Individual Device Tests ===\n");
    uint8_t test_addresses[] = {0x3C, 0x48, 0x58, 0x68, 0x76};
    uint8_t num_test_addr = sizeof(test_addresses) / sizeof(test_addresses[0]);
    
    for (uint8_t i = 0; i < num_test_addr; i++) {
        uint8_t addr = test_addresses[i];
        bool found = i2c_probe_device(addr);
        printf("Device at 0x%X: %s\n", addr, found ? "FOUND" : "NOT FOUND");
    }

    // SGP30传感器测试
    printf("\n=== SGP30 Air Quality Sensor Test ===\n");
    
    // 读取设备信息
    uint64_t serial_id;
    uint16_t feature_set;
    
    if (sgp30_read_serial_id(&serial_id)) {
        printf("SGP30 Serial ID: 0x%X %X\n", serial_id >> 32, serial_id & 0xFFFFFFFF);
    } else {
        printf("Failed to read SGP30 serial ID\n");
    }
    
    if (sgp30_read_feature_set(&feature_set)) {
        printf("SGP30 Feature Set: 0x%X\n", feature_set);
    } else {
        printf("Failed to read SGP30 feature set\n");
    }
    
    // 执行自测试
    uint16_t test_result;
    if (sgp30_measure_test(&test_result)) {
        printf("SGP30 Self-test result: 0x%X %s\n", 
               test_result, (test_result == 0xD400) ? "(PASS)" : "(FAIL)");
    } else {
        printf("SGP30 Self-test failed\n");
    }
    
    // 初始化空气质量测量
    printf("\nInitializing SGP30 air quality measurement...\n");
    if (sgp30_init_air_quality()) {
        printf("SGP30 initialization successful\n");
        
        // 设置湿度补偿 (假设50%相对湿度)
        uint16_t humidity = 50 * 256;  // 50% RH * 256
        if (sgp30_set_humidity(humidity)) {
            printf("Humidity compensation set to 50%% RH\n");
        }
        
        // 连续测量空气质量
        printf("\nStarting continuous air quality measurements...\n");
        printf("Note: SGP30 needs 15 seconds to stabilize readings\n");
        
        for (int i = 0; i < 10; i++) {
            sgp30_air_quality_t air_quality;
            
            if (sgp30_measure_air_quality(&air_quality)) {
                printf("Measurement %d: CO2eq=%d ppm, TVOC=%d ppb\n", 
                       i+1, air_quality.co2_eq_ppm, air_quality.tvoc_ppb);
            } else {
                printf("Measurement %d: Failed to read air quality\n", i+1);
            }
            if(air_quality.co2_eq_ppm == 400 && air_quality.tvoc_ppb == 0){
                i=0;
                printf("SGP30 is not ready, please wait...\n");
                delay_s(10);
                continue;
            }
            // 等待1秒再进行下次测量
            delay_ms(1000);
        }
        
        // 读取基线值
        sgp30_baseline_t baseline;
        if (sgp30_get_baseline(&baseline)) {
            printf("\nCurrent baseline values:\n");
            printf("  CO2eq baseline: 0x%04X\n", baseline.co2_eq_baseline);
            printf("  TVOC baseline: 0x%04X\n", baseline.tvoc_baseline);
            printf("Note: Save these values for faster startup next time\n");
        }
        
    } else {
        printf("SGP30 initialization failed\n");
    }

    printf("\n=== All Tests Complete ===\n");
}
