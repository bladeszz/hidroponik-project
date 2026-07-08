// ============================================================
//  SENSORS.INO - Sensör Fonksiyonları
//  TDS Hesaplama, Su Seviye Okuma, Medyan Filtreleme
// ============================================================

// TDS Medyan Filtreleme Algoritması
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++) bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0) bTemp = bTab[(iFilterLen - 1) / 2];
  else bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}

// Su Seviyesi Okuma (TDS kapatılarak)
int readWaterLevel() {
  readingWaterLevel = true;
  digitalWrite(TDS_POWER_PIN, LOW);
  delay(200);
  digitalWrite(WATER_POWER_PIN, HIGH);
  delay(100);

  long waterSum = 0;
  for (int i = 0; i < 5; i++) {
    waterSum += analogRead(WATER_SIGNAL_PIN);
    delay(20);
  }
  int value = waterSum / 5;

  digitalWrite(WATER_POWER_PIN, LOW);
  delay(50);
  digitalWrite(TDS_POWER_PIN, HIGH);
  delay(200);
  readingWaterLevel = false;

  return value;
}

// TDS Hesaplama (kalibrasyon sabitleri config.h'de)
float calculateTDS() {
  int tdsBufferTemp[TDS_SAMPLE_COUNT];  // Lokal kopya (sort için)
  for (int i = 0; i < TDS_SAMPLE_COUNT; i++) {
    tdsBufferTemp[i] = tdsBuffer[i];
  }
  float avgVoltage = getMedianNum(tdsBufferTemp, TDS_SAMPLE_COUNT) * VREF / 4096.0;

  // Sıcaklık kompanzasyonu (DHT okunamadıysa 25°C varsayılır = kompanzasyonsuz)
  float tempForComp = isnan(temperature) ? 25.0 : temperature;
  float compensationCoefficient = 1.0 + 0.02 * (tempForComp - 25.0);
  float compensationVoltage = avgVoltage / compensationCoefficient;

  return (TDS_COEFF_A * pow(compensationVoltage, 3)
        - TDS_COEFF_B * pow(compensationVoltage, 2)
        + TDS_COEFF_C * compensationVoltage) * TDS_FACTOR;
}