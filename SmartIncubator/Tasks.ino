// FreeRTOS tasks for sensing, control, safety, display, timing, and networking.

void SensorReadTask(void *parameters) {
  TickType_t lastWakeTime = xTaskGetTickCount();

  while (true) {
    SensorData data{};
    data.temperature = dht.readTemperature();
    data.humidity = dht.readHumidity();
    data.timestamp = millis();
    data.valid = !isnan(data.temperature) && !isnan(data.humidity);

    xQueueOverwrite(sensorQueue, &data);

    Serial.print("Temperature: ");
    Serial.print(data.temperature);
    Serial.print(" C   Humidity: ");
    Serial.print(data.humidity);
    Serial.print(" %   Valid: ");
    Serial.println(data.valid);

    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(Config::SENSOR_PERIOD_MS));
  }
}

void HeaterCtrlTask(void *parameters) {
  SensorData data{};
  TickType_t lastWakeTime = xTaskGetTickCount();

  while (true) {
    if (manualMode) {
      if (manualHeaterRequested && !safetyFault) {
        heaterOnSafe();
      } else {
        heaterOff();
      }
    } else if (
        xQueuePeek(sensorQueue, &data, 0) == pdTRUE &&
        data.valid &&
        !safetyFault) {
      float lowLimit;
      float highLimit;
      getTemperatureLimits(lowLimit, highLimit);

      // Preserve the current output while the reading is inside the hysteresis band.
      if (data.temperature < lowLimit) {
        heaterOnSafe();
      } else if (data.temperature > highLimit) {
        heaterOff();
      }
    } else {
      heaterOff();
    }

    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(Config::HEATER_PERIOD_MS));
  }
}

void HumidityCtrlTask(void *parameters) {
  SensorData data{};
  TickType_t lastWakeTime = xTaskGetTickCount();

  while (true) {
    if (manualMode) {
      if (manualFanRequested && !safetyFault) {
        humidityOnSafe();
      } else {
        humidityOff();
      }
    } else if (
        xQueuePeek(sensorQueue, &data, 0) == pdTRUE &&
        data.valid &&
        !safetyFault) {
      float lowLimit;
      float highLimit;
      getHumidityLimits(lowLimit, highLimit);

      if (data.humidity < lowLimit) {
        humidityOnSafe();
      } else if (data.humidity > highLimit) {
        humidityOff();
      }
    } else {
      humidityOff();
    }

    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(Config::HUMIDITY_PERIOD_MS));
  }
}

void turnTimerCallback(TimerHandle_t timer) {
  if (!lockdown && !manualMode) {
    xTaskNotifyGive(eggTurnTaskHandle);
  }
}

void EggTurnTask(void *parameters) {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if (lockdown) {
      Serial.println("Egg turn ignored: LOCKDOWN");
      continue;
    }

    if (xSemaphoreTake(motorMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
      continue;
    }

    eggTurningNow = true;

    if (servoPositionA) {
      eggServo.write(Config::SERVO_POSITION_B);
      servoPositionA = false;
      Serial.println("Egg tray -> Position B");
    } else {
      eggServo.write(Config::SERVO_POSITION_A);
      servoPositionA = true;
      Serial.println("Egg tray -> Position A");
    }

    vTaskDelay(pdMS_TO_TICKS(Config::TURN_SETTLE_MS));
    eggTurningNow = false;
    xSemaphoreGive(motorMutex);
  }
}

void SafetyWatchdogTask(void *parameters) {
  SensorData data{};
  TickType_t lastWakeTime = xTaskGetTickCount();
  uint32_t rangeOutSince = 0;

  while (true) {
    const bool hasReading = xQueuePeek(sensorQueue, &data, 0) == pdTRUE;
    const bool immediateFault =
        !hasReading ||
        !data.valid ||
        millis() - data.timestamp > Config::SENSOR_STALE_MS ||
        data.temperature < -10.0F ||
        data.temperature > 80.0F ||
        data.humidity < 0.0F ||
        data.humidity > 100.0F ||
        data.temperature > 42.0F;

    if (immediateFault) {
      safetyFault = true;
      alarmActive = true;
      heaterOff();
      humidityOff();
      digitalWrite(Config::Pins::BUZZER, HIGH);
      rangeOutSince = 0;
    } else {
      safetyFault = false;

      float tempLow;
      float tempHigh;
      float humidityLow;
      float humidityHigh;
      getTemperatureLimits(tempLow, tempHigh);
      getHumidityLimits(humidityLow, humidityHigh);

      const bool outOfRange =
          data.temperature < tempLow ||
          data.temperature > tempHigh ||
          data.humidity < humidityLow ||
          data.humidity > humidityHigh;

      bool rangeAlarm = false;
      if (Config::ENABLE_RANGE_ALARMS && outOfRange) {
        if (rangeOutSince == 0) {
          rangeOutSince = millis();
        }
        rangeAlarm = millis() - rangeOutSince >= Config::RANGE_ALARM_GRACE_MS;
      } else {
        rangeOutSince = 0;
      }

      alarmActive = rangeAlarm;
      digitalWrite(Config::Pins::BUZZER, alarmActive ? HIGH : LOW);
    }

    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(Config::WATCHDOG_PERIOD_MS));
  }
}

void DisplayTask(void *parameters) {
  SensorData data{};
  TickType_t lastWakeTime = xTaskGetTickCount();

  while (true) {
    if (
        xQueuePeek(sensorQueue, &data, 0) == pdTRUE &&
        xSemaphoreTake(displayMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);

      display.setCursor(0, 0);
      display.print("DAY ");
      display.print(incubationDay);
      display.print("/");
      display.print(Config::TOTAL_INCUBATION_DAYS);

      display.setCursor(0, 15);
      display.print("Temp: ");
      if (data.valid) {
        display.print(data.temperature, 1);
      } else {
        display.print("ERR");
      }
      display.print(" C");

      display.setCursor(0, 28);
      display.print("Hum : ");
      if (data.valid) {
        display.print(data.humidity, 0);
      } else {
        display.print("ERR");
      }
      display.print(" %");

      display.setCursor(0, 42);
      display.print("Heat:");
      display.print(heaterOn ? "ON " : "OFF");

      display.setCursor(65, 42);
      display.print("Hum:");
      display.print(humidifierOn ? "ON" : "OFF");

      display.setCursor(0, 55);
      if (alarmActive) {
        display.print("ALARM");
      } else if (manualMode) {
        display.print("MANUAL");
      } else if (lockdown) {
        display.print("LOCKDOWN");
      } else {
        display.print("AUTO");
      }

      display.display();
      xSemaphoreGive(displayMutex);
    }

    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(Config::DISPLAY_PERIOD_MS));
  }
}

void DayCounterTask(void *parameters) {
  const uint32_t startTime = millis();

  while (true) {
    const uint32_t elapsedDays = (millis() - startTime) / 86400000UL;
    incubationDay = 1 + elapsedDays;

    if (incubationDay >= Config::LOCKDOWN_DAY) {
      lockdown = true;
    }
    if (incubationDay > Config::TOTAL_INCUBATION_DAYS) {
      incubationDay = Config::TOTAL_INCUBATION_DAYS;
    }

    vTaskDelay(pdMS_TO_TICKS(60000));
  }
}

void BlynkSyncTask(void *parameters) {
  SensorData data{};
  uint32_t lastPublish = 0;
  uint32_t lastWiFiTry = 0;
  uint32_t lastBlynkTry = 0;

  while (true) {
    if (WiFi.status() != WL_CONNECTED) {
      if (millis() - lastWiFiTry >= 10000UL) {
        lastWiFiTry = millis();
        Serial.println("WiFi connection retry...");
        WiFi.disconnect();
        vTaskDelay(pdMS_TO_TICKS(100));
        logConfiguredNetworkVisibility();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      }
    } else if (!Blynk.connected()) {
      if (millis() - lastBlynkTry >= 5000UL) {
        lastBlynkTry = millis();
        Serial.println("Blynk reconnecting...");
        Blynk.connect(1000);
      }
    } else {
      Blynk.run();

      if (millis() - lastPublish >= Config::BLYNK_PUBLISH_PERIOD_MS) {
        lastPublish = millis();

        if (xQueuePeek(sensorQueue, &data, 0) == pdTRUE && data.valid) {
          Blynk.virtualWrite(V0, data.temperature);
          Blynk.virtualWrite(V1, data.humidity);
        }

        Blynk.virtualWrite(V2, heaterOn ? 1 : 0);
        Blynk.virtualWrite(V3, humidifierOn ? 1 : 0);
        Blynk.virtualWrite(V4, eggTurningNow ? 1 : 0);
        Blynk.virtualWrite(V5, incubationDay);
        Blynk.virtualWrite(V9, alarmActive ? "ALARM" : "OK");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
