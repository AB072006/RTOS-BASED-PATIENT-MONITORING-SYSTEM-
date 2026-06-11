# 🏥 RTOS-Based Multi-Parameter Patient Monitor

> A real-time, deterministic patient monitoring system built on the **NXP LPC1768 (ARM Cortex-M3)** microcontroller using **FreeRTOS**, capable of simultaneously monitoring Heart Rate, SpO₂, and Body Temperature with guaranteed alarm response times.

---

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [System Architecture](#system-architecture)
- [RTOS Task Design](#rtos-task-design)
- [Circuit Connections](#circuit-connections)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
- [UART Output Format](#uart-output-format)
- [Fault Tolerance & Reliability](#fault-tolerance--reliability)
- [Future Scope](#future-scope)
- [References](#references)

---

## Overview

Traditional bare-metal embedded systems suffer from **timing bottlenecks** — a processor busy updating a display cannot simultaneously poll a sensor, potentially missing a life-critical alarm. This project resolves that by migrating to a **FreeRTOS preemptive multitasking architecture**, where tasks are independently scheduled based on strict priority levels.

The system continuously captures:
- **Heart Rate (BPM)** and **SpO₂ (%)** via the MAX30102 sensor over I2C
- **Body Temperature (°C)** via the DS18B20 sensor over 1-Wire

All data is displayed on a **16×2 LCD** and simultaneously streamed over **UART** for real-time remote monitoring.

---

## Features

- ✅ **Preemptive multitasking** — FreeRTOS scheduler ensures critical tasks always preempt low-priority ones
- ✅ **Deterministic alarm response** — O(1) interrupt latency via ARM Cortex-M3 NVIC
- ✅ **Thread-safe data sharing** — FreeRTOS Queues prevent race conditions between tasks
- ✅ **Software watchdog** — `vTaskMonitor` detects and reports task hangs within 5 seconds
- ✅ **Stack overflow protection** — `configCHECK_FOR_STACK_OVERFLOW` enabled
- ✅ **Zero task starvation** — verified through Keil µVision simulation
- ✅ **Modular and scalable** — new sensors can be added as independent tasks

---

## Hardware Requirements

| Component | Description |
|---|---|
| **NXP LPC1768** | ARM Cortex-M3 MCU @ 100 MHz (main controller) |
| **MAX30102** | Pulse Oximeter & Heart Rate Sensor (I2C) |
| **DS18B20** | Digital Temperature Sensor (1-Wire) |
| **16×2 LCD** | Character display (4-bit parallel mode) |
| **Buzzer / LED** | Alarm output (connected to GPIO P1.19) |
| **USB-to-UART (CP2102)** | For UART serial monitoring on PC |
| **Power Supply** | 3.3V / 5V via onboard LDO regulator |

---

## Software Requirements

| Tool | Purpose |
|---|---|
| **Keil µVision 4 / 5** | IDE for compilation, debugging, and simulation |
| **FreeRTOS V10+** | Real-Time Operating System kernel |
| **CMSIS / LPC17xx Driver** | Hardware abstraction for LPC1768 peripherals |
| **Serial Monitor** | PuTTY / Tera Term / Arduino Serial Monitor (115200 baud) |

---

## System Architecture

```
┌─────────────────────────────────────────────────────┐
│               Patient Data Acquisition               │
│  [DS18B20]    [MAX30102]    [ECG Front-End]          │
│  1-Wire Bus   I2C Bus       ADC Interface            │
└──────────┬───────────┬──────────────┬───────────────┘
           │           │              │
┌──────────▼───────────▼──────────────▼───────────────┐
│            LPC1768 ARM Cortex-M3 (RTOS Core)        │
│                                                      │
│   [vTaskSensors P3] ──► xVitalsQueue                │
│   [vTaskDisplay P2] ◄── xVitalsQueue                │
│   [vTaskUART    P1] ◄── xVitalsQueue (peek)         │
│   [vTaskMonitor P2] ──► Watchdog Supervision        │
└──────────────────────────────┬──────────────────────┘
                               │
        ┌──────────────────────┼───────────────────┐
        ▼                      ▼                   ▼
   [16×2 LCD]          [UART → PC Monitor]   [Buzzer/LED Alarm]
```

---

## RTOS Task Design

| Task Name | Priority | Period | Responsibility |
|---|---|---|---|
| `vTaskSensors` | **3 (Highest)** | 1000 ms | I2C + 1-Wire sensor acquisition, queue push |
| `vTaskMonitor` | **2 (Medium)** | 5000 ms | Software watchdog, heartbeat validation |
| `vTaskDisplay` | **2 (Medium)** | Event-driven | LCD UI update via queue receive |
| `vTaskUART` | **1 (Lowest)** | 500 ms | Serial telemetry to PC |

### Why This Priority Order?

- `vTaskSensors` at Priority 3 ensures **sensor polling is never blocked** by display or logging operations.
- `vTaskMonitor` at Priority 2 acts as a **safety supervisor** — it runs frequently enough to catch crashes within 5 seconds without interfering with sensor acquisition.
- `vTaskUART` at Priority 1 ensures **heavy string formatting never delays** a critical medical reading.

---

## Circuit Connections

### MAX30102 → LPC1768 (I2C)

```
MAX30102 SDA  →  LPC1768 P0.27 (SDA0)
MAX30102 SCL  →  LPC1768 P0.28 (SCL0)
MAX30102 VIN  →  3.3V
MAX30102 GND  →  GND
```

### DS18B20 → LPC1768 (1-Wire)

```
DS18B20 DATA  →  LPC1768 P0.05 (with 4.7kΩ pull-up to 3.3V)
DS18B20 VDD   →  3.3V
DS18B20 GND   →  GND
```

### 16×2 LCD → LPC1768 (4-bit Parallel)

```
LCD RS   →  P1.0
LCD EN   →  P1.1
LCD D4   →  P1.9
LCD D5   →  P1.10
LCD D6   →  P1.14
LCD D7   →  P1.15
LCD VDD  →  5V
LCD GND  →  GND
LCD RW   →  GND (always write mode)
```

### Buzzer / Alarm

```
Buzzer(+)  →  LPC1768 P1.19
Buzzer(-)  →  GND
```

### UART (Serial Monitor)

```
LPC1768 TXD0 (P0.2)  →  CP2102 RXD
LPC1768 GND          →  CP2102 GND
```

---

## Project Structure

```
RTOS-Patient-Monitor/
│
├── main.c                  # Core application — tasks, queue, scheduler
├── max30102.h / .c         # MAX30102 I2C driver
├── ds18b20.h / .c          # DS18B20 1-Wire driver
│
├── FreeRTOS/               # FreeRTOS kernel source
│   ├── include/
│   ├── portable/
│   │   └── RVDS/ARM_CM3/   # Cortex-M3 port
│   ├── tasks.c
│   ├── queue.c
│   └── list.c
│
├── FreeRTOSConfig.h        # RTOS configuration (tick rate, stack sizes, etc.)
├── LPC17xx.h               # CMSIS device header
│
├── docs/
│   └── LPC1768_PROJECT_24BEC502.pdf   # Project report
│
└── README.md
```

---

## Getting Started

### Step 1 — Clone the Repository

```bash
git clone https://github.com/<your-username>/RTOS-Patient-Monitor.git
cd RTOS-Patient-Monitor
```

### Step 2 — Open in Keil µVision

1. Open Keil µVision 4 or 5.
2. Go to **Project → Open Project** and select the `.uvprojx` file.
3. Ensure the **Device** is set to `NXP LPC1768`.

### Step 3 — Add FreeRTOS to the Project

If FreeRTOS source is not already included:
1. Download [FreeRTOS V10+](https://www.freertos.org/a00104.html).
2. Copy the following into your project's `FreeRTOS/` folder:
   - `tasks.c`, `queue.c`, `list.c`, `timers.c`
   - `include/` headers
   - `portable/RVDS/ARM_CM3/port.c` + `portmacro.h`
   - `portable/MemMang/heap_4.c`
3. Add all these files to the Keil project tree.

### Step 4 — Configure FreeRTOSConfig.h

Key settings to verify:

```c
#define configUSE_PREEMPTION              1
#define configCPU_CLOCK_HZ                100000000UL   // 100 MHz
#define configTICK_RATE_HZ                1000          // 1ms tick
#define configMAX_PRIORITIES              5
#define configMINIMAL_STACK_SIZE          128
#define configCHECK_FOR_STACK_OVERFLOW    2             // Enable stack guard
#define configUSE_IDLE_HOOK               0
#define configUSE_TICK_HOOK               0
```

### Step 5 — Build and Flash

1. Click **Build** (F7) — resolve any include path issues if needed.
2. Connect LPC1768 via USB.
3. Click **Download** (F8) to flash the firmware.

### Step 6 — Monitor Output

Open your serial terminal (PuTTY / Tera Term) with:

```
Port   : COMx (check Device Manager)
Baud   : 115200
Data   : 8N1
```

---

## UART Output Format

The system streams vitals every **500 ms** in the following CSV format:

```
75,98,36
76,97,36
74,98,37
...
```

Format: `HeartRate(BPM), SpO2(%), Temperature(°C)`

You can plot this in **real-time** using Python + matplotlib:

```python
import serial
import matplotlib.pyplot as plt

ser = serial.Serial('COM3', 115200)
hr_data, spo2_data, temp_data = [], [], []

while True:
    line = ser.readline().decode().strip()
    hr, spo2, temp = map(int, line.split(','))
    hr_data.append(hr)
    spo2_data.append(spo2)
    temp_data.append(temp)
    # Update plot...
```

---

## Fault Tolerance & Reliability

| Mechanism | Implementation |
|---|---|
| **Software Watchdog** | `vTaskMonitor` checks task heartbeat counters every 5s; triggers buzzer + UART alert on stall |
| **Stack Overflow Guard** | `configCHECK_FOR_STACK_OVERFLOW = 2` monitors bounds on every context switch |
| **Atomic Queue Operations** | `xQueueOverwrite` ensures Display and Alarm always read the **latest** vital packet, never stale data |
| **Task Isolation** | Each task has its own independent stack — a crash in `vTaskUART` cannot corrupt `vTaskSensors` |
| **Fail-Safe State** | On detected kernel stall: buzzer asserted + "CRITICAL ERROR" broadcast over UART |

---

## Future Scope

- **ECG / NIBP Integration** — Add as independent FreeRTOS tasks without affecting existing timing
- **IoT Gateway** — Replace UART task with ESP8266/GSM for MQTT-based cloud telemetry (IoMT)
- **Arrhythmia Detection** — Implement FFT/Wavelet analysis in a dedicated DSP task on Cortex-M3
- **Low-Power Mode** — Use `vApplicationIdleHook` to enter sleep state between sensor polls
- **Priority Ceiling Protocol** — Prevent priority inversion with hardware WDT as secondary protection

---

## References

1. Real Time Engineers Ltd., "FreeRTOS API Reference," 2024 — https://www.freertos.org/a00106.html
2. NXP Semiconductors, "UM10360: LPC176x/5x User Manual," Rev. 3.1, Apr. 2014
3. Analog Devices, "MAX30102 Datasheet — High-Sensitivity Pulse Oximeter," Rev. 1, 2018
4. ARM Limited, "Cortex-M3 Generic User Guide," ARM DUI 0552A, 2010
5. T. Tamura et al., "Wearable Photoplethysmographic Sensors — Past and Present," *Electronics*, vol. 3, no. 2, Apr. 2014

---

## Author

**Anubhav Bhavsar** — 24BEC502  
Department of Electronics and Communication Engineering  
Institute of Technology, Nirma University  
Course: Embedded Systems (3EC705CC24) — Semester VI

---

> *"Migrating from bare-metal to RTOS is not just an engineering choice — for medical devices, it is a patient safety decision."*
