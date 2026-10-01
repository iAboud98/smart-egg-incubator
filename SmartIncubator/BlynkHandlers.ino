// Blynk dashboard commands. Manual outputs remain subject to local safety checks.

BLYNK_WRITE(V6) {
  manualMode = param.asInt() == 1;
  manualHeaterRequested = false;
  manualFanRequested = false;

  heaterOff();
  humidityOff();
  Blynk.virtualWrite(V10, 0);
  Blynk.virtualWrite(V11, 0);

  Serial.print("Blynk Manual Mode: ");
  Serial.println(manualMode ? "ON" : "OFF");
}

BLYNK_WRITE(V7) {
  const float value = param.asFloat();
  if (value >= Config::MANUAL_TEMP_MIN_C && value <= Config::MANUAL_TEMP_MAX_C) {
    manualTempSetpoint = value;
  }

  Serial.print("Manual Temp Setpoint: ");
  Serial.println(manualTempSetpoint);
}

BLYNK_WRITE(V8) {
  if (manualMode && param.asInt() == 1 && eggTurnTaskHandle != nullptr) {
    Serial.println("Blynk: Manual egg turn");
    xTaskNotifyGive(eggTurnTaskHandle);
  }
}

BLYNK_WRITE(V10) {
  if (manualMode) {
    manualHeaterRequested = param.asInt() == 1;
  } else {
    manualHeaterRequested = false;
    Blynk.virtualWrite(V10, 0);
  }

  Serial.print("Blynk Manual Heater Request: ");
  Serial.println(manualHeaterRequested ? "ON" : "OFF");
}

BLYNK_WRITE(V11) {
  if (manualMode) {
    manualFanRequested = param.asInt() == 1;
  } else {
    manualFanRequested = false;
    Blynk.virtualWrite(V11, 0);
  }

  Serial.print("Blynk Manual Fan Request: ");
  Serial.println(manualFanRequested ? "ON" : "OFF");
}

BLYNK_CONNECTED() {
  Serial.println("Blynk connected");
  Blynk.syncVirtual(V6);
  Blynk.syncVirtual(V7);

  // A reconnect never restores a previous manual actuator command.
  manualHeaterRequested = false;
  manualFanRequested = false;
  Blynk.virtualWrite(V10, 0);
  Blynk.virtualWrite(V11, 0);
}
