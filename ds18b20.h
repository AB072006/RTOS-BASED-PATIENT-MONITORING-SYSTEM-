/* ==========================================================================
 * Sensor:  DS18B20 (Digital Temperature Sensor)
 * Protocol: 1-Wire
 * Pin:     P1.0 (As physically wired on the RDL Trainer Kit)
 * ========================================================================== */

#ifndef DS18B20_H
#define DS18B20_H

#include "LPC17xx.h"
#include <stdint.h>

// --- Hardware Pin Definition ---
// Mapped to Port 1, Pin 0
#define DS18B20_PIN (1 << 0) 

// --- 1-Wire Command Codes ---
#define DS18B20_CMD_SKIP_ROM       0xCC
#define DS18B20_CMD_CONVERT_T      0x44
#define DS18B20_CMD_READ_SCRATCH   0xBE

// --- Core Function Prototypes ---
// These are the exact functions your FreeRTOS tasks call in main.c
void DS18B20_Init(void);
int DS18B20_Get_Temp(void);

// --- Low-Level 1-Wire Protocol Functions (To be written in ds18b20.c) ---
void DS18B20_Delay_us(uint32_t us);
uint8_t DS18B20_Reset(void);
void DS18B20_WriteBit(uint8_t bit);
uint8_t DS18B20_ReadBit(void);
void DS18B20_WriteByte(uint8_t byte);
uint8_t DS18B20_ReadByte(void);

#endif /* DS18B20_H */

