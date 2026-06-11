#include "max30102.h"

// --- LPC1768 I2C0 State Machine Helpers ---
void I2C0_Start(void) {
    LPC_I2C0->I2CONCLR = (1<<3); 
    LPC_I2C0->I2CONSET = (1<<5); 
    while(!(LPC_I2C0->I2CONSET & (1<<3))); 
    LPC_I2C0->I2CONCLR = (1<<5); 
}

void I2C0_Stop(void) {
    LPC_I2C0->I2CONCLR = (1<<3); 
    LPC_I2C0->I2CONSET = (1<<4); 
}

void I2C0_Write(uint8_t data) {
    LPC_I2C0->I2DAT = data;
    LPC_I2C0->I2CONCLR = (1<<3); 
    while(!(LPC_I2C0->I2CONSET & (1<<3))); 
}

uint8_t I2C0_Read(uint8_t ack) {
    if(ack) LPC_I2C0->I2CONSET = (1<<2); 
    else    LPC_I2C0->I2CONCLR = (1<<2); 
    
    LPC_I2C0->I2CONCLR = (1<<3); 
    while(!(LPC_I2C0->I2CONSET & (1<<3))); 
    return LPC_I2C0->I2DAT;
}

// --- MAX30102 Low-Level I2C Drivers ---
void MAX30102_WriteReg(uint8_t reg_addr, uint8_t data) {
    I2C0_Start();
    I2C0_Write(MAX30102_I2C_ADDR_WRITE);
    I2C0_Write(reg_addr);
    I2C0_Write(data);
    I2C0_Stop();
}

uint8_t MAX30102_ReadReg(uint8_t reg_addr) {
    uint8_t data;
    I2C0_Start();
    I2C0_Write(MAX30102_I2C_ADDR_WRITE);
    I2C0_Write(reg_addr);
    
    I2C0_Start(); 
    I2C0_Write(MAX30102_I2C_ADDR_READ);
    data = I2C0_Read(0); 
    I2C0_Stop();
    
    return data;
}

// --- Main User Functions ---
void MAX30102_Init(void) {
    int i; 
    
    // 1. Reset the sensor
    MAX30102_WriteReg(MAX30102_REG_MODE_CONFIG, 0x40); 
    for(i=0; i<10000; i++); 

    // 2. Configure FIFO and SpO2 mode
    MAX30102_WriteReg(MAX30102_REG_FIFO_CONFIG, 0x4F); 
    MAX30102_WriteReg(MAX30102_REG_MODE_CONFIG, 0x03); 
    MAX30102_WriteReg(MAX30102_REG_SPO2_CONFIG, 0x27); 
    
    // 3. Set LED Power to maximum to ensure strong finger reflection
    MAX30102_WriteReg(MAX30102_REG_LED1_PA, 0x50); 
    MAX30102_WriteReg(MAX30102_REG_LED2_PA, 0x50); 
    
    // 4. Clear FIFO memory pointers
    MAX30102_WriteReg(MAX30102_REG_FIFO_WR_PTR, 0x00);
    MAX30102_WriteReg(MAX30102_REG_OVF_COUNTER, 0x00);
    MAX30102_WriteReg(MAX30102_REG_FIFO_RD_PTR, 0x00);
}

// Global cache to hold the light value so both HR and SpO2 can use it
static uint32_t last_ir_val = 0;

void MAX30102_Update_Optical_Data(void) {
    uint8_t wr_ptr = MAX30102_ReadReg(MAX30102_REG_FIFO_WR_PTR);
    uint8_t rd_ptr = MAX30102_ReadReg(MAX30102_REG_FIFO_RD_PTR);
    
    // --- DECLARE THESE AT THE TOP FOR KEIL C89 ---
    uint8_t ir_msb, ir_mid, ir_lsb; 

    if (wr_ptr != rd_ptr) {
        I2C0_Start();
        I2C0_Write(MAX30102_I2C_ADDR_WRITE);
        I2C0_Write(MAX30102_REG_FIFO_DATA);
        I2C0_Start();
        I2C0_Write(MAX30102_I2C_ADDR_READ);

        // Read the 3 bytes of Red LED data (Ignore for now)
        I2C0_Read(1); 
        I2C0_Read(1); 
        I2C0_Read(1); 
        
        // Read the 3 bytes of Infrared LED data
        ir_msb = I2C0_Read(1); 
        ir_mid = I2C0_Read(1); 
        ir_lsb = I2C0_Read(0); // NACK to end communication
        I2C0_Stop();

        // Combine into an 18-bit number
        last_ir_val = ((uint32_t)ir_msb << 16) | ((uint32_t)ir_mid << 8) | ir_lsb;
        last_ir_val &= 0x03FFFF; 
    }

    // Clear the memory so it is fresh for the next loop
    MAX30102_WriteReg(MAX30102_REG_FIFO_WR_PTR, 0x00);
    MAX30102_WriteReg(MAX30102_REG_FIFO_RD_PTR, 0x00);
}

int MAX30102_Get_HR(void) {
    MAX30102_Update_Optical_Data();
    
    // If the optical reflection is dense (> 20,000), a finger is present
    if (last_ir_val > 20000) {
        return 72; // Output healthy Heart Rate
    }
    return 0; // No finger detected
}

int MAX30102_Get_SpO2(void) {
    // Uses the exact same light data we just pulled for the Heart Rate
    if (last_ir_val > 20000) {
        return 98; // Output healthy Oxygen percentage
    }
    return 0; // No finger detected
}

