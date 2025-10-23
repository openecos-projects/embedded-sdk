#include "main.h"
#include "i2c_scanner.h"

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
