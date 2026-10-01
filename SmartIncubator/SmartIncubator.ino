// Smart incubator entry point, shared state, and system initialization.

#include "Secrets.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <Wire.h>

#include "Config.h"

struct SensorData {
  float temperature;
  float humidity;
  uint32_t timestamp;
  bool valid;
};

DHT dht(Config::Pins::DHT, Config::DHT_TYPE);
Servo eggServo;
Adafruit_SSD1306 display(
    Config::OLED_WIDTH,
    Config::OLED_HEIGHT,
    &Wire,
    -1);

QueueHandle_t sensorQueue = nullptr;
SemaphoreHandle_t displayMutex = nullptr;
SemaphoreHandle_t motorMutex = nullptr;
TimerHandle_t turnTimer = nullptr;
TaskHandle_t eggTurnTaskHandle = nullptr;

volatile bool heaterOn = false;
volatile bool humidifierOn = false;
volatile bool lockdown = false;
volatile int incubationDay = 1;
volatile bool servoPositionA = true;
volatile bool eggTurningNow = false;
volatile bool manualMode = false;
volatile float manualTempSetpoint = 37.5F;
volatile bool manualHeaterRequested = false;
volatile bool manualFanRequested = false;
volatile bool safetyFault = true;
volatile bool alarmActive = false;

void setup() {
  Serial.begin(115200);

  // Put both active-low relays in their fail-safe state before enabling GPIO output.
  digitalWrite(Config::Pins::HEATER_RELAY, Config::RELAY_OFF);
  digitalWrite(Config::Pins::HUMIDITY_RELAY, Config::RELAY_OFF);
  pinMode(Config::Pins::HEATER_RELAY, OUTPUT);
  pinMode(Config::Pins::HUMIDITY_RELAY, OUTPUT);

  pinMode(Config::Pins::BUZZER, OUTPUT);
  digitalWrite(Config::Pins::BUZZER, LOW);

  dht.begin();

  Wire.begin(Config::Pins::OLED_SDA, Config::Pins::OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, Config::OLED_ADDRESS)) {
    Serial.println("OLED INITIALIZATION FAILED");
    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("SMART INCUBATOR");
  display.setCursor(0, 35);
  display.println("Starting...");
  display.display();

  eggServo.attach(Config::Pins::SERVO);
  eggServo.write(Config::SERVO_POSITION_A);

  sensorQueue = xQueueCreate(1, sizeof(SensorData));
  displayMutex = xSemaphoreCreateMutex();
  motorMutex = xSemaphoreCreateMutex();

  // Networking starts asynchronously so local control never waits for Wi-Fi.
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  logConfiguredNetworkVisibility();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Blynk.config(BLYNK_AUTH_TOKEN);

  // Time-critical local tasks run on core 1; networking is isolated on core 0.
  xTaskCreatePinnedToCore(SensorReadTask, "Sensor", 4096, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(HeaterCtrlTask, "Heater", 2048, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(HumidityCtrlTask, "Humidity", 2048, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(
      EggTurnTask, "EggTurn", 2048, nullptr, 2, &eggTurnTaskHandle, 1);
  xTaskCreatePinnedToCore(SafetyWatchdogTask, "Watchdog", 3072, nullptr, 4, nullptr, 1);
  xTaskCreatePinnedToCore(DisplayTask, "Display", 4096, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(DayCounterTask, "DayCounter", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(BlynkSyncTask, "Blynk", 6144, nullptr, 2, nullptr, 0);

  turnTimer = xTimerCreate(
      "TurnTimer",
      pdMS_TO_TICKS(Config::TURN_INTERVAL_MS),
      pdTRUE,
      nullptr,
      turnTimerCallback);

  if (turnTimer != nullptr) {
    xTimerStart(turnTimer, 0);
  }

  Serial.println();
  Serial.println("================================");
  Serial.println("SMART INCUBATOR STARTED");
  Serial.println("Local FreeRTOS control active");
  Serial.println("WiFi/Blynk running separately");
  Serial.println("================================");
}

void loop() {
  // Application work is performed by the FreeRTOS tasks.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
