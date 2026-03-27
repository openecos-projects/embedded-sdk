#ifndef GPIO_COMMON_H
#define GPIO_COMMON_H

#include <stdint.h>

typedef enum {
    GPIO_NUM_0 = 0,
    GPIO_NUM_1 = 1,
    GPIO_NUM_2 = 2,
    GPIO_NUM_3 = 3,
    GPIO_NUM_4 = 4,
    GPIO_NUM_5 = 5,
    GPIO_NUM_6 = 6,
    GPIO_NUM_7 = 7,
    GPIO_NUM_8 = 8,
    GPIO_NUM_9 = 9,
    GPIO_NUM_10 = 10,
    GPIO_NUM_11 = 11,
    GPIO_NUM_12 = 12,
    GPIO_NUM_13 = 13,
    GPIO_NUM_14 = 14,
    GPIO_NUM_15 = 15,
    GPIO_NUM_16 = 16,
    GPIO_NUM_17 = 17,
    GPIO_NUM_18 = 18,
    GPIO_NUM_19 = 19,
    GPIO_NUM_20 = 20,
    GPIO_NUM_21 = 21,
    GPIO_NUM_22 = 22,
    GPIO_NUM_23 = 23,
    GPIO_NUM_24 = 24,
    GPIO_NUM_25 = 25,
    GPIO_NUM_26 = 26,
    GPIO_NUM_27 = 27,
    GPIO_NUM_28 = 28,
    GPIO_NUM_29 = 29,
    GPIO_NUM_30 = 30,
    GPIO_NUM_31 = 31,
    GPIO_NUM_32 = 32,
    GPIO_NUM_33 = 33,
    GPIO_NUM_34 = 34,
    GPIO_NUM_35 = 35,
    GPIO_NUM_36 = 36,
    GPIO_NUM_37 = 37,
    GPIO_NUM_38 = 38,
    GPIO_NUM_39 = 39,
    GPIO_NUM_40 = 40,
    GPIO_NUM_41 = 41,
    GPIO_NUM_42 = 42,
    GPIO_NUM_43 = 43,
    GPIO_NUM_44 = 44,
    GPIO_NUM_45 = 45,
    GPIO_NUM_46 = 46,
    GPIO_NUM_47 = 47,
    GPIO_NUM_48 = 48,
    GPIO_NUM_49 = 49,
    GPIO_NUM_50 = 50,
    GPIO_NUM_51 = 51,
    GPIO_NUM_52 = 52,
    GPIO_NUM_53 = 53,
    GPIO_NUM_54 = 54,
    GPIO_NUM_55 = 55,
    GPIO_NUM_56 = 56,
    GPIO_NUM_57 = 57,
    GPIO_NUM_58 = 58,
    GPIO_NUM_59 = 59,
    GPIO_NUM_60 = 60,
    GPIO_NUM_61 = 61,
    GPIO_NUM_62 = 62,
    GPIO_NUM_63 = 63,
} gpio_num_t;

typedef enum{
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT = 1,
} gpio_mode_t;

typedef enum{
    GPIO_LEVEL_LOW = 0,
    GPIO_LEVEL_HIGH = 1,
} gpio_level_t;

typedef enum{
    GPIO_FUNCTION_0 = 0,
    GPIO_FUNCTION_1 = 1,
    GPIO_FUNCTION_2 = 2,
} gpio_func_t;

#endif