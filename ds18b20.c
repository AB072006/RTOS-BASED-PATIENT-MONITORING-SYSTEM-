#include "ds18b20.h"
#include "FreeRTOS.h"
#include "task.h"

// --- Microsecond Delay ---
// Approximated for LPC1768 running at 100MHz
void DS18B20_Delay_us(uint32_t us) {
    uint32_t count = us * 12; 
    while(count--) {
        __asm("nop"); // No Operation (keeps the processor busy)
    }
}

// --- Pin Control Helpers ---
void DS18B20_Pin_Output(void) { LPC_GPIO1->FIODIR |= DS18B20_PIN; }
void DS18B20_Pin_Input(void)  { LPC_GPIO1->FIODIR &= ~DS18B20_PIN; }
void DS18B20_Pin_Low(void)    { LPC_GPIO1->FIOCLR = DS18B20_PIN; }
uint8_t DS18B20_Pin_Read(void){ return (LPC_GPIO1->FIOPIN & DS18B20_PIN) ? 1 : 0; }

// --- 1-Wire Protocol ---
uint8_t DS18B20_Reset(void) {
    uint8_t presence;
    DS18B20_Pin_Output();
    DS18B20_Pin_Low();
    DS18B20_Delay_us(480); // Pull low for 480us to reset
    
    DS18B20_Pin_Input();   // Release line (4.7k resistor pulls it high)
    DS18B20_Delay_us(70);  // Wait for sensor response
    
    presence = DS18B20_Pin_Read(); // 0 = Sensor Present, 1 = Error
    DS18B20_Delay_us(410);
    
    return presence; 
}

void DS18B20_WriteBit(uint8_t bit) {
    DS18B20_Pin_Output();
    DS18B20_Pin_Low();
    DS18B20_Delay_us(2);
    if (bit) {
        DS18B20_Pin_Input(); // If writing '1', release early
    }
    DS18B20_Delay_us(60);
    DS18B20_Pin_Input();
    DS18B20_Delay_us(2);
}

uint8_t DS18B20_ReadBit(void) {
    uint8_t bit = 0;
    DS18B20_Pin_Output();
    DS18B20_Pin_Low();
    DS18B20_Delay_us(2);
    
    DS18B20_Pin_Input(); // Release line
    DS18B20_Delay_us(10); // Sample after 10us
    
    bit = DS18B20_Pin_Read();
    DS18B20_Delay_us(50);
    return bit;
}

void DS18B20_WriteByte(uint8_t byte) {
    int i; // <--- Declare 'i' at the top of the function
    for (i = 0; i < 8; i++) { 
        DS18B20_WriteBit(byte & 0x01);
        byte >>= 1;
    }
}

uint8_t DS18B20_ReadByte(void) {
    uint8_t byte = 0;
    int i; // <--- Declare 'i' at the top of the function
    for (i = 0; i < 8; i++) { 
        if (DS18B20_ReadBit()) {
            byte |= (1 << i);
        }
    }
    return byte;
}

// --- Main User Functions ---
void DS18B20_Init(void) {
    DS18B20_Pin_Input(); // Ensure pin is floating/pulled up initially
}

int DS18B20_Get_Temp(void) {
    uint8_t temp_lsb, temp_msb;
    int16_t temp_raw;

    if (DS18B20_Reset() != 0) return 99; // Return error 99C if sensor is unplugged

    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_CONVERT_T);

    // FreeRTOS delay while sensor processes the temperature (takes ~750ms)
    vTaskDelay(pdMS_TO_TICKS(750));

    DS18B20_Reset();
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_READ_SCRATCH);

    temp_lsb = DS18B20_ReadByte();
    temp_msb = DS18B20_ReadByte();

    // Combine bytes
    temp_raw = (temp_msb << 8) | temp_lsb;
    
    // DS18B20 outputs in 1/16th of a degree Celsius
    return (temp_raw / 16);
}

