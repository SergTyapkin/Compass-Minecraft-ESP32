#pragma once

// Button
#define BTN_PIN 5                      // Пин кнопки

// LED matrix
#define LED_PIN 0                      // Пин для сигнала в LED-матрицу
#define LED_COUNT 64                   // Количество светодиодов (8×8)
#define LED_MATRIX_WIDTH 8             // Ширина матрицы (отдельно, потому что она может быть сложной формы. Это размеры описанного прямоугольника вокруг неё)
#define LED_MATRIX_HEIGHT 8            // Высота матрицы

// Pins I2C (Magnitometer)
#define I2C_SDA_PIN 8                  // SDA для магнитометра
#define I2C_SCL_PIN 9                  // SCL для магнитометра
#define I2C_MAG_ADDR 0x2C              // I2C адрес QMC5883L (обычно 0x0D)

// Pins UART (GPS)
#define GPS_UART_PIN_RX 20             // RX ESP32 (подключается к TX GPS)
#define GPS_UART_PIN_TX 21             // TX ESP32 (подключается к RX GPS)

// Pins Battery Monitor (TP4056 plate)
#define BATTERY_CHARGING_PIN 2         // Пин индикации зарядки с TP4056 (LOW = заряжается)
#define BATTERY_FULL_PIN 1             // Пин индикации заряда с TP4056 (LOW = заряжена)

// Calibration parameters
#define CALIBRATION_MIN_TIME_MS 10000
#define CALIBRATION_MIN_PERCENT 0.75
