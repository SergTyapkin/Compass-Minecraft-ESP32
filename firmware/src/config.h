#pragma once

// Button
#define BTN_PIN 5                         // Пин кнопки

// LED matrix
#define LED_PIN 0                        // Пин для сигнала в LED-матрицу
#define LED_COUNT 64                     // Количество светодиодов (8×8)
#define LED_MATRIX_WIDTH 8               // Ширина матрицы (отдельно, потому что она может быть сложной формы. Это размеры описанного прямоугольника вокруг неё)
#define LED_MATRIX_HEIGHT 8              // Высота матрицы

// Pins I2C (Magnitometer)
#define I2C_SDA_PIN 8                    // SDA для магнитометра
#define I2C_SCL_PIN 9                    // SCL для магнитометра
#define I2C_MAG_ADDR 0x2C                // I2C адрес QMC5883L (обычно 0x0D)

// GPS pins and distance
#define GPS_UART_PIN_RX 20               // RX ESP32 (подключается к TX GPS)
#define GPS_UART_PIN_TX 21               // TX ESP32 (подключается к RX GPS)
#define GPS_DISTANCE_TO_POINT_REACHED_METERS 3 // Расстояние до точки, чтобы она считалась достигнутой

// Buzzer
#define BUZZER_PIN 4                     // Пин пьезоэлемента (пищалки)
#define BUZZER_PWM_CHANNEL 1             // Ну надо так
#define BUZZER_PWM_RESOLUTION 8          // Ну надо так
#define BUZZER_TONE_DURATION 150         // ms на одну ноту музыки

// Pins Battery Monitor (TP4056 plate)
#define BATTERY_CHARGING_PIN 2           // Пин индикации зарядки с TP4056 (LOW = заряжается)
#define BATTERY_FULL_PIN 1               // Пин индикации заряда с TP4056 (LOW = заряжена)

// Calibration parameters
#define CALIBRATION_MIN_TIME_MS 10000
#define CALIBRATION_MIN_PERCENT 0.75

// Filtering
#define MAG_FILTER_SIZE 15               // Размер медианного фильтра для магнитометра

// Time manager settings
#define WIFI_SSID_NAME "Sergs-Archer-E204"    // Название сети Wifi
#define WIFI_PASSWORD "<YOUR_WIFI_PASSWORD>"  // Пароль от Wifi
#define NTP_SERVER_HOST "pool.ntp.org"   // Хост NTP сервера для синхронизации времени
#define GMT_OFFSET_SEC (3 * 60 * 60)     // Смещение часового пояса GMT+3
#define DAYLIGHT_OFFSET_SEC 0            // Смещение летнего/зимнего времени

// Controls
#define BRIGHTNESS_STEP 5                // Шаг яркости при зажатии кнопки
#define BRIGHTNESS_MIN  1                // Минимальное значение яркости
#define BRIGHTNESS_MAX  255              // Максимальное значение яркости

// Visuals
#define COMPASS_ROTATION_OFFSET_RAD (PI)    // Смещение при выводе фреймов компаса (чтобы направление севера магнитометра совпадало со стрелкой)
// 🔁 Calibration progressbar params
#define PROGRESS_BAR_HEGIHT 4                                         // Ширина прогрессбара (по вертикали. Не меньше 4)
#define PROGRESS_BAR_WIDTH LED_MATRIX_WIDTH                           // Длина прогрессбара (по вертикали. Не меньше 5)
#define PROGRESS_BAR_Y ((LED_MATRIX_HEIGHT - PROGRESS_BAR_HEGIHT) / 2)  // Y позиция, откуда рисуем прогрессбар
// 🔋🟡 Charging animantion params
// | Corpus
#define ANIMATION_BATTERY_HUE_INITIAL_DEG 32                 // Средний оттенок в HSV для заряжающейся батарейки (30 - желтый)
#define ANIMATION_BATTERY_HUE_AMPLITUDE_DEG 10               // Амплитуда анимации оттенка для заряжающейся батарейки
#define ANIMATION_BATTERY_HUE_ANIMATION_SPEED 0.001          // Скорость изменения оттенка для заряжающейся батарейки
#define ANIMATION_BATTERY_HUE_BRIGHTNESS cfg.brightness      // Яркость анимации заряжающейся батарейки
// | Filling
#define ANIMATION_WAVE_WIDTH 5                               // Длина изредка пробегающей волны зарядки
#define ANIMATION_WAVE_MAX_POSITION 15                       // Дальнее положение пробегающей волны зарядки
#define ANIMATION_WAVE_STEP_MS 30                            // Скорость пробегания волны
#define ANIMATION_WAVE_MAX_BRIGHTNESS cfg.brightness         // Яркость волны
#define ANIMATION_WAVE_HUE_DEG 50                            // Оттенок в HSV для волны (50 - между желтым и зеленым)
// 🔋🟢 Charged animantion params
#define ANIMATION_BATTERY_FULL_HUE_INITIAL_DEG 90            // Средний оттенок в HSV для заряженной батарейки (90 - зеленый)
#define ANIMATION_BATTERY_FULL_HUE_AMPLITUDE_DEG 15          // Амплитуда анимации оттенка для заряженной батарейки
#define ANIMATION_BATTERY_FULL_HUE_ANIMATION_SPEED 0.001     // Скорость изменения оттенка для заряженной батарейки
#define ANIMATION_BATTERY_FULL_HUE_BRIGHTNESS cfg.brightness // Яркость анимации заряженной батарейки
// 🕓 Clock
#define CLOCK_MATRIX_X 0       // Начало отрисовки часов на матрице по X
#define CLOCK_MATRIX_Y 0       // Начало отрисовки часов на матрице по Y
#define CLOCK_MATRIX_WIDTH 7   // Ширина области матрицы для часов. Лучше нечетные числа
#define CLOCK_MATRIX_HEIGHT 7  // Высота области матрицы для часов. Лучше нечетные числа
// 🌐 GPS
#define GPS_MATRIX_X 0       // Начало отрисовки gps компаса на матрице по X
#define GPS_MATRIX_Y 0       // Начало отрисовки gps компаса на матрице по Y
#define GPS_MATRIX_WIDTH 7   // Ширина области матрицы для gps компаса
#define GPS_MATRIX_HEIGHT 7  // Высота области матрицы для gps компаса