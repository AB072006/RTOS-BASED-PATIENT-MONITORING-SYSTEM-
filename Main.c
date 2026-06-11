#include "LPC17xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

// --- Hardware Drivers ---
#include "max30102.h"  
#include "ds18b20.h"   


// --- LCD Pins (P5 Header) ---
#define LCD_RS  (1 << 0)
#define LCD_EN  (1 << 1)
#define LCD_D4  (1 << 9)
#define LCD_D5  (1 << 10)
#define LCD_D6  (1 << 14)
#define LCD_D7  (1 << 15)

/* Global Heartbeat Counters for Research-Grade Reliability */
volatile uint32_t ulSensorTaskHeartbeat = 0;
volatile uint32_t ulDisplayTaskHeartbeat = 0;

// Structure for Queue Data
typedef struct {
    int hr;
    int spo2;
    int temp;
} Vitals_t;

QueueHandle_t xVitalsQueue;

void UART0_SendString(char *str);

// 1. LCD RTOS DRIVER
void LCD_SendNibble(unsigned char n) {
    LPC_GPIO1->FIOCLR = (LCD_D4|LCD_D5|LCD_D6|LCD_D7);
    if(n & 0x01) LPC_GPIO1->FIOSET = LCD_D4;
    if(n & 0x02) LPC_GPIO1->FIOSET = LCD_D5;
    if(n & 0x04) LPC_GPIO1->FIOSET = LCD_D6;
    if(n & 0x08) LPC_GPIO1->FIOSET = LCD_D7;
    LPC_GPIO1->FIOSET = LCD_EN;
    vTaskDelay(pdMS_TO_TICKS(2)); // RTOS-friendly delay
    LPC_GPIO1->FIOCLR = LCD_EN;
}

void LCD_Cmd(unsigned char c) {
    LPC_GPIO1->FIOCLR = LCD_RS;
    LCD_SendNibble(c >> 4);
    LCD_SendNibble(c & 0x0F);
}

void LCD_Dat(unsigned char d) {
    LPC_GPIO1->FIOSET = LCD_RS;
    LCD_SendNibble(d >> 4);
    LCD_SendNibble(d & 0x0F);
}

void LCD_Init_RTOS(void) {
    LPC_GPIO1->FIODIR |= (LCD_RS|LCD_EN|LCD_D4|LCD_D5|LCD_D6|LCD_D7);
    vTaskDelay(pdMS_TO_TICKS(50));
    LCD_Cmd(0x33); LCD_Cmd(0x32); LCD_Cmd(0x28);
    LCD_Cmd(0x0C); LCD_Cmd(0x06); LCD_Cmd(0x01);
}


// 2. RTOS TASKS
// TASK 1: SENSOR ACQUISITION (Highest Priority)
void vTaskSensors(void *pvParameters) {
    Vitals_t data = {75, 98, 36};
    for(;;) {
				ulSensorTaskHeartbeat++;
        // Simulated Random Walk Logic
        data.hr += (rand() % 3) - 1;
        data.spo2 += (rand() % 3) - 1;
        data.temp = 36 + (rand() % 2); // Slight fluctuation
        
        // Bounds checking
        if(data.hr < 60) data.hr = 60; if(data.hr > 100) data.hr = 100;

        // PUSH data to Queue
        xQueueOverwrite(xVitalsQueue, &data); 
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Run every 1 second
			
    }
}

// TASK 2: DISPLAY MANAGER (Medium Priority)
void vTaskDisplay(void *pvParameters) {
    Vitals_t rxData;
		int i;
    char buf[16];
    LCD_Init_RTOS();
    
    for(;;) {
			  ulDisplayTaskHeartbeat++;
        // Wait for data from Queue
        if(xQueueReceive(xVitalsQueue, &rxData, portMAX_DELAY) == pdPASS) {
            LCD_Cmd(0x80);
            sprintf(buf, "HR:%3d SpO2:%d%%", rxData.hr, rxData.spo2);
            for(i=0; buf[i]; i++) LCD_Dat(buf[i]);
            
            LCD_Cmd(0xC0);
            sprintf(buf, "Temp: %d C      ", rxData.temp);
            for(i=0; buf[i]; i++) LCD_Dat(buf[i]);
        }
    }
}

// TASK 3: UART TELEMETRY (Lowest Priority)
void vTaskUART(void *pvParameters) {
    Vitals_t txData;
		int i;
    char uartBuf[32];
    // UART Init
    LPC_PINCON->PINSEL0 |= (1 << 4) | (1 << 6); 
    LPC_UART0->LCR = 0x83; LPC_UART0->DLL = 162; LPC_UART0->LCR = 0x03;
		
    for(;;) {
        if(xQueuePeek(xVitalsQueue, &txData, 0) == pdPASS) {
            sprintf(uartBuf, "%d,%d,%d\r\n", txData.hr, txData.spo2, txData.temp);
            for(i=0; uartBuf[i]; i++) {
                while(!(LPC_UART0->LSR & (1<<5)));
                LPC_UART0->THR = uartBuf[i];
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vTaskMonitor(void *pvParameters) {
    uint32_t lastSensorCount = 0;
    uint32_t lastDisplayCount = 0;
    
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(5000)); // Check every 5 seconds
        
        // Reliability Check: Has the sensor task crashed?
        if(ulSensorTaskHeartbeat == lastSensorCount) {
            // SYSTEM FAILURE DETECTED: Trigger Buzzer and UART Alert
            LPC_GPIO1->FIOSET = (1 << 19); 
            UART0_SendString("CRITICAL ERROR: Sensor Task Hang Detected!\r\n");
        }
        
				if(ulDisplayTaskHeartbeat == lastDisplayCount) 
					{
							UART0_SendString("WARNING: Display Task Stall!\r\n");
					}
				
        lastSensorCount = ulSensorTaskHeartbeat;
        lastDisplayCount = ulDisplayTaskHeartbeat;
    }
}


// 3. MAIN BOOTLOADER
int main(void) {
    SystemInit();
    
    // Create the Queue (The 'List' of data packets)
    xVitalsQueue = xQueueCreate(1, sizeof(Vitals_t));

    if(xVitalsQueue != NULL) {
        // Create Tasks (Proves Scheduling)
        xTaskCreate(vTaskSensors, "Sensors", 128, NULL, 3, NULL);
        xTaskCreate(vTaskDisplay, "Display", 128, NULL, 2, NULL);
        xTaskCreate(vTaskUART,    "UART",    128, NULL, 1, NULL);
				xTaskCreate(vTaskMonitor, "Monitor", 128, NULL, 2, NULL);
        // Start the RTOS Scheduler (The Proof!)
        vTaskStartScheduler();
    }

    for(;;); // Should never reach here\\Dead End (Safety Net)
}

void UART0_SendString(char *str) {
    int i; 
    for(i = 0; str[i] != '\0'; i++) {
        // Wait until THR is empty
        while(!(LPC_UART0->LSR & (1 << 5))); 
        // Send the character
        LPC_UART0->THR = str[i]; 
    }
}

