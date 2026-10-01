#pragma once

#include <Arduino.h>

// Hardware mapping and operating parameters for the incubator.
namespace Config {

namespace Pins {
constexpr uint8_t DHT = 4;
constexpr uint8_t SERVO = 18;
constexpr uint8_t OLED_SDA = 21;
constexpr uint8_t OLED_SCL = 22;
constexpr uint8_t BUZZER = 25;
constexpr uint8_t HEATER_RELAY = 26;   // Relay K1
constexpr uint8_t HUMIDITY_RELAY = 27; // Relay K2 / fan
} // namespace Pins

constexpr uint8_t DHT_TYPE = 11; // DHT11

constexpr uint8_t RELAY_ON = LOW;
constexpr uint8_t RELAY_OFF = HIGH;

constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;

constexpr bool ENABLE_HEATER_OUTPUT = true;
constexpr bool ENABLE_RANGE_ALARMS = false;

constexpr float TEMP_LOW_C = 37.2F;
constexpr float TEMP_HIGH_C = 37.8F;
constexpr float MANUAL_TEMP_MIN_C = 35.0F;
constexpr float MANUAL_TEMP_MAX_C = 40.0F;
constexpr float MANUAL_TEMP_HYSTERESIS_C = 0.3F;

constexpr float HUMIDITY_SETTING_LOW = 50.0F;
constexpr float HUMIDITY_SETTING_HIGH = 55.0F;
constexpr float HUMIDITY_LOCKDOWN_LOW = 65.0F;
constexpr float HUMIDITY_LOCKDOWN_HIGH = 70.0F;

constexpr int TOTAL_INCUBATION_DAYS = 21;
constexpr int LOCKDOWN_DAY = 18;

constexpr int SERVO_POSITION_A = 0;
constexpr int SERVO_POSITION_B = 260;

constexpr uint32_t SENSOR_PERIOD_MS = 1000UL;
constexpr uint32_t HEATER_PERIOD_MS = 1000UL;
constexpr uint32_t HUMIDITY_PERIOD_MS = 2000UL;
constexpr uint32_t WATCHDOG_PERIOD_MS = 500UL;
constexpr uint32_t DISPLAY_PERIOD_MS = 1000UL;
constexpr uint32_t BLYNK_PUBLISH_PERIOD_MS = 2000UL;
constexpr uint32_t SENSOR_STALE_MS = 5000UL;
constexpr uint32_t RANGE_ALARM_GRACE_MS = 120000UL;
constexpr uint32_t TURN_INTERVAL_MS = 4UL * 60UL * 60UL * 1000UL;
constexpr uint32_t TURN_SETTLE_MS = 2000UL;

} // namespace Config
