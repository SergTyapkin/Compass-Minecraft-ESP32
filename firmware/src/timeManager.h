#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <Preferences.h>


// Умные часы с NTP, RTC и сохранением в NVS
class TimeManager {
private:
    // ----- Настройки -----
    String wifiSsid;
    String wifiPassword;
    String ntpServerHost;
    long   gmtOffsetSec;
    int    daylightOffsetSec;
    bool   isUseStoredTime;        // использовать сохранённое время при старте?

    // ----- Состояние -----
    bool   isNtpSynced = false;
    unsigned long lastSyncAttempt = 0;
    unsigned long lastSyncSuccess = 0;

    // ----- Внутренний RTC (программный) -----
    time_t rtcStartTime = 0;
    unsigned long rtcStartMs = 0;

    // ----- Хранилище (Preferences) -----
    Preferences preferences;
    static const char* NVS_NAMESPACE;  // "time_keeper"

    // ----- Константы -----
    static const unsigned long SYNC_INTERVAL_MS = 60 * 60 * 1000;  // 1 час
    static const unsigned long RETRY_INTERVAL_MS = 5 * 60 * 1000;  // 5 минут
    static const unsigned long MAX_CONNECTION_ATTEMPTS = 10;       // 10 раз

    // ======== ПРИВАТНЫЕ МЕТОДЫ ========

    // ---------- Wi-Fi ----------
    bool connectWiFi() {
        if (WiFi.status() == WL_CONNECTED) return true;

        Serial.println("📡 Подключение к Wi-Fi...");
        WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());

        int currentAttempts = 0;
        while (WiFi.status() != WL_CONNECTED && currentAttempts < MAX_CONNECTION_ATTEMPTS) {
            delay(500);
            Serial.print(".");
            currentAttempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n✅ Wi-Fi подключен!");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
            return true;
        } else {
            Serial.println("\n❌ Не удалось подключиться к Wi-Fi");
            return false;
        }
    }

    // ---------- NTP синхронизация ----------
    bool syncWithNTP() {
        if (!connectWiFi()) return false;

        Serial.println("🔄 Синхронизация с NTP сервером...");
        configTime(gmtOffsetSec, daylightOffsetSec, ntpServerHost.c_str());

        struct tm timeinfo;
        int currenAtttempts = 0;
        while (!getLocalTime(&timeinfo) && currenAtttempts < MAX_CONNECTION_ATTEMPTS) {
            delay(500);
            Serial.printf("Попытка синхронизации %d/%d...\n", currenAtttempts, MAX_CONNECTION_ATTEMPTS);
            currenAtttempts++;
        }

        if (currenAtttempts < MAX_CONNECTION_ATTEMPTS) {
            isNtpSynced = true;
            lastSyncSuccess = millis();
            time_t now = time(nullptr);
            rtcStartTime = now;
            rtcStartMs = millis();

            // Сохраняем в NVS
            saveTimeToNVS(now);

            Serial.println("\n✅ Время синхронизировано!");
            Serial.print(&timeinfo, "%A, %B %d %Y %H:%M:%S");
            Serial.println();
            return true;
        } else {
            Serial.println("\n❌ Не удалось синхронизировать время с NTP");
            return false;
        }
    }

    // ---------- Работа с NVS (энергонезависимая память) ----------
    void saveTimeToNVS(time_t timestamp) {
        preferences.begin(NVS_NAMESPACE, false);
        preferences.putLong("saved_time", (long)timestamp);
        preferences.putBool("has_time", true);
        preferences.end();
        Serial.printf("💾 Время сохранено в NVS: %ld\n", (long)timestamp);
    }

    bool loadTimeFromNVS(time_t& outTimestamp) {
        preferences.begin(NVS_NAMESPACE, true);  // read-only
        bool has = preferences.getBool("has_time", false);
        if (has) {
            outTimestamp = (time_t)preferences.getLong("saved_time", 0);
            preferences.end();
            return true;
        }
        preferences.end();
        return false;
    }

    void clearNVS() {
        preferences.begin(NVS_NAMESPACE, false);
        preferences.clear();
        preferences.end();
        Serial.println("🗑️ NVS очищен");
    }

    // ---------- Программный RTC ----------
    time_t getRTCTime() {
        unsigned long elapsedMs = millis() - rtcStartMs;
        return rtcStartTime + (elapsedMs / 1000);
    }

    void setRTCStartTime(time_t timestamp) {
        rtcStartTime = timestamp;
        rtcStartMs = millis();
    }

public:
    // ======== КОНСТРУКТОР ========
    TimeManager(const String& wifiSsid,
                const String& wifiPassword,
                const String& ntpServerHost = "pool.ntp.org",
                long gmtOffsetSec = 0,      // GMT+0 (Гринвич)
                int daylightOffsetSec = 0,
                bool useStoredTime = true)
        : wifiSsid(wifiSsid)
        , wifiPassword(wifiPassword)
        , ntpServerHost(ntpServerHost)
        , gmtOffsetSec(gmtOffsetSec)
        , daylightOffsetSec(daylightOffsetSec)
        , isUseStoredTime(useStoredTime) {
        
        rtcStartTime = 0;
        rtcStartMs = millis();
        NVS_NAMESPACE = "time_keeper";
    }

    // ======== ИНИЦИАЛИЗАЦИЯ ========
    void begin() {
        Serial.println("\n⏰ Инициализация TimeManager...");

        // 1. Пробуем загрузить сохранённое время из NVS
        time_t savedTime = 0;
        bool hasSaved = false;
        if (isUseStoredTime) {
            hasSaved = loadTimeFromNVS(savedTime);
            if (hasSaved && savedTime > 0) {
                setRTCStartTime(savedTime);
                Serial.printf("⏳ Загружено сохранённое время: %s\n", formatDateTime(savedTime).c_str());
            } else {
                Serial.printf("⚠️  В NVS нет сохранённого времени. Начинаем с (1970-01-01) \n");
            }
        }

        // 2. Пробуем синхронизироваться с NTP
        if (syncWithNTP()) {
            Serial.println("✅ NTP синхронизация успешна!");
        } else {
            if (hasSaved && savedTime > 0) {
                Serial.println("⚠️ NTP не доступен, используем сохранённое время");
                setRTCStartTime(savedTime);
                isNtpSynced = false;
            } else {
                Serial.println("⚠️ NTP не доступен, время не установлено (1970-01-01)");
                setRTCStartTime(0);
                isNtpSynced = false;
            }
        }
    }

    // ======== ПОЛУЧЕНИЕ ВРЕМЕНИ ========

    // Текущее время в формате time_t
    time_t getTime() {
        if (isNtpSynced) {
            return time(nullptr);
        }
        return getRTCTime();
    }

    // Структура tm
    struct tm getLocalTimeStruct() {
        time_t now = getTime();
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        return timeinfo;
    }

    // Форматированная строка
    String getTimeString(const String& format = "%H:%M:%S") {
        struct tm timeinfo = getLocalTimeStruct();
        char buffer[32];
        strftime(buffer, sizeof(buffer), format.c_str(), &timeinfo);
        return String(buffer);
    }

    String getDateString(const String& format = "%d.%m.%Y") {
        struct tm timeinfo = getLocalTimeStruct();
        char buffer[32];
        strftime(buffer, sizeof(buffer), format.c_str(), &timeinfo);
        return String(buffer);
    }

    String getDateTimeString() {
        struct tm timeinfo = getLocalTimeStruct();
        char buffer[64];
        strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", &timeinfo);
        return String(buffer);
    }

    // Форматирование time_t в строку
    String formatDateTime(time_t timestamp) {
        struct tm timeinfo;
        localtime_r(&timestamp, &timeinfo);
        char buffer[64];
        strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", &timeinfo);
        return String(buffer);
    }

    // ======== РУЧНАЯ УСТАНОВКА ВРЕМЕНИ ========

    // Установка времени из time_t
    bool setTime(time_t timestamp) {
        if (timestamp <= 0) return false;
        
        setRTCStartTime(timestamp);
        isNtpSynced = false;  // больше не доверяем системному времени
        saveTimeToNVS(timestamp);
        
        Serial.printf("🕐 Время установлено вручную: %s\n", formatDateTime(timestamp).c_str());
        return true;
    }

    // Установка времени из строки "DD.MM.YYYY HH:MM:SS"
    bool setDateTimeFromString(const String& dateTimeStr) {
        // Парсим строку вида "31.12.2025 23:59:59"
        int day, month, year, hour, minute, second;
        if (sscanf(dateTimeStr.c_str(), "%d.%d.%d %d:%d:%d",
                   &day, &month, &year, &hour, &minute, &second) == 6) {
            
            struct tm tm = {0};
            tm.tm_mday = day;
            tm.tm_mon  = month - 1;
            tm.tm_year = year - 1900;
            tm.tm_hour = hour;
            tm.tm_min  = minute;
            tm.tm_sec  = second;
            
            time_t timestamp = mktime(&tm);
            if (timestamp > 0) {
                return setTime(timestamp);
            }
        }
        Serial.println("❌ Ошибка парсинга даты. Используйте формат: DD.MM.YYYY HH:MM:SS");
        return false;
    }

    // Установка только даты (время устанавливается на 00:00:00)
    bool setDateFromString(const String& dateStr) {
        int day, month, year;
        if (sscanf(dateStr.c_str(), "%d.%d.%d", &day, &month, &year) == 3) {
            struct tm tm = {0};
            tm.tm_mday = day;
            tm.tm_mon  = month - 1;
            tm.tm_year = year - 1900;
            tm.tm_hour = 0;
            tm.tm_min  = 0;
            tm.tm_sec  = 0;
            
            time_t timestamp = mktime(&tm);
            if (timestamp > 0) {
                return setTime(timestamp);
            }
        }
        Serial.println("❌ Ошибка парсинга даты. Используйте формат: DD.MM.YYYY");
        return false;
    }

    // Установка только времени (дата остаётся текущей)
    bool setTimeFromString(const String& timeStr) {
        int hour, minute, second = 0;
        if (sscanf(timeStr.c_str(), "%d:%d:%d", &hour, &minute, &second) >= 2) {
            struct tm tm = getLocalTimeStruct();
            tm.tm_hour = hour;
            tm.tm_min  = minute;
            tm.tm_sec  = second;
            
            time_t timestamp = mktime(&tm);
            if (timestamp > 0) {
                return setTime(timestamp);
            }
        }
        Serial.println("❌ Ошибка парсинга времени. Используйте формат: HH:MM или HH:MM:SS");
        return false;
    }

    // ======== ДОПОЛНИТЕЛЬНЫЕ МЕТОДЫ ========

    bool getIsNtpSynced() const {
        return isNtpSynced;
    }

    // Принудительная синхронизация с NTP
    bool forceSync() {
        bool result = syncWithNTP();
        if (result) {
            // Сохраняем свежее время
            saveTimeToNVS(time(nullptr));
        }
        return result;
    }

    // Сохранить текущее время в NVS (на случай, если хотите сделать это вручную)
    void saveCurrentTime() {
        time_t now = getTime();
        if (now > 0) {
            saveTimeToNVS(now);
        }
    }

    // Получить сохранённое время из NVS (без запуска RTC)
    time_t getStoredTime() {
        time_t t = 0;
        loadTimeFromNVS(t);
        return t;
    }

    // Очистить сохранённое время
    void clearStoredTime() {
        clearNVS();
    }
};

// Статическая переменная (определение вне класса)
const char* TimeManager::NVS_NAMESPACE = "time_keeper";