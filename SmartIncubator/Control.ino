// Fail-safe actuator helpers and active incubation limits.

void heaterOff() {
  digitalWrite(Config::Pins::HEATER_RELAY, Config::RELAY_OFF);
  heaterOn = false;
}

void heaterOnSafe() {
  if (Config::ENABLE_HEATER_OUTPUT && !safetyFault) {
    digitalWrite(Config::Pins::HEATER_RELAY, Config::RELAY_ON);
    heaterOn = true;
    return;
  }

  heaterOff();
}

void humidityOff() {
  digitalWrite(Config::Pins::HUMIDITY_RELAY, Config::RELAY_OFF);
  humidifierOn = false;
}

void humidityOnSafe() {
  if (!safetyFault) {
    digitalWrite(Config::Pins::HUMIDITY_RELAY, Config::RELAY_ON);
    humidifierOn = true;
    return;
  }

  humidityOff();
}

void getTemperatureLimits(float &lowLimit, float &highLimit) {
  if (manualMode) {
    lowLimit = manualTempSetpoint - Config::MANUAL_TEMP_HYSTERESIS_C;
    highLimit = manualTempSetpoint + Config::MANUAL_TEMP_HYSTERESIS_C;
    return;
  }

  lowLimit = Config::TEMP_LOW_C;
  highLimit = Config::TEMP_HIGH_C;
}

void getHumidityLimits(float &lowLimit, float &highLimit) {
  if (lockdown) {
    lowLimit = Config::HUMIDITY_LOCKDOWN_LOW;
    highLimit = Config::HUMIDITY_LOCKDOWN_HIGH;
    return;
  }

  lowLimit = Config::HUMIDITY_SETTING_LOW;
  highLimit = Config::HUMIDITY_SETTING_HIGH;
}

void logConfiguredNetworkVisibility() {
  Serial.print("Looking for WiFi network: ");
  Serial.println(WIFI_SSID);

  const int networkCount = WiFi.scanNetworks();
  bool targetFound = false;

  for (int i = 0; i < networkCount; ++i) {
    if (WiFi.SSID(i) == WIFI_SSID) {
      targetFound = true;
      Serial.print("Hotspot found, signal: ");
      Serial.print(WiFi.RSSI(i));
      Serial.println(" dBm");
      break;
    }
  }

  if (!targetFound) {
    Serial.println("Hotspot not visible (ESP32 requires 2.4 GHz WiFi)");
  }

  WiFi.scanDelete();
}
