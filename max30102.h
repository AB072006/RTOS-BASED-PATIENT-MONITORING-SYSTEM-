/* ==========================================================================
 * Sensor:  MAX30102 (Pulse Oximeter & Heart Rate)
 * Protocol: I2C (P0.27 SDA, P0.28 SCL)
 * ========================================================================== */

#ifndef MAX30102_H
#define MAX30102_H

#include "LPC17xx.h"
#include <stdint.h>

// --- I2C Address ---
// The 7-bit I2C address for MAX30102 is 0x57. 
// Shifted left by 1 for ARM Cortex I2C registers: 0xAE (Write) and 0xAF (Read)
#define MAX30102_I2C_ADDR_WRITE 0xAE
#define MAX30102_I2C_ADDR_READ  0xAF

// --- Core Register Addresses ---
#define MAX30102_REG_INTR_STATUS_1  0x00
#define MAX30102_REG_INTR_ENABLE_1  0x02
#define MAX30102_REG_FIFO_WR_PTR    0x04
#define MAX30102_REG_OVF_COUNTER    0x05
#define MAX30102_REG_FIFO_RD_PTR    0x06
#define MAX30102_REG_FIFO_DATA      0x07
#define MAX30102_REG_FIFO_CONFIG    0x08
#define MAX30102_REG_MODE_CONFIG    0x09
#define MAX30102_REG_SPO2_CONFIG    0x0A
#define MAX30102_REG_LED1_PA        0x0C // Red LED
#define MAX30102_REG_LED2_PA        0x0D // IR LED

// --- Core Function Prototypes ---
// These are the exact functions your FreeRTOS tasks call in main.c
void MAX30102_Init(void);
int MAX30102_Get_HR(void);
int MAX30102_Get_SpO2(void);

// --- Low-Level I2C Helper Functions (To be written in max30102.c) ---
void MAX30102_WriteReg(uint8_t reg_addr, uint8_t data);
uint8_t MAX30102_ReadReg(uint8_t reg_addr);
void MAX30102_ReadFifo(uint32_t *red_led, uint32_t *ir_led);

#endif /* MAX30102_H */
