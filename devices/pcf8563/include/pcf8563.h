#ifndef PCF8563_H
#define PCF8563_H

#include "pcf8563_type.h"

// Initialize the PCF8563 if needed (this just checks basic comms typically)
void pcf8563_init(pcf8563_device_t* dev);

// Write current time info to the RTC
void pcf8563_write_info(pcf8563_device_t* dev, const pcf8563_info_t *info);

// Read current time info from the RTC
pcf8563_info_t pcf8563_read_info(pcf8563_device_t* dev);

#endif
