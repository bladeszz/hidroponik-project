// ============================================================
//  RELAYS.INO - Röle Kontrol ve Zamanlama Motoru
//  LED ve Pompa rölelerini zamanlama/override'a göre günceller
// ============================================================

// "HH:MM" formatını dakikaya çevir (00:00=0, 23:59=1439)
int timeToMinutes(const char* hhmm) {
  int h = 0, m = 0;
  if (sscanf(hhmm, "%d:%d", &h, &m) == 2) {
    return h * 60 + m;
  }
  return -1;  // Geçersiz format
}

// Mevcut RTC saatinin LED zamanlaması içinde olup olmadığını kontrol et
bool isLedScheduleActive(int nowMin) {
  int startMin = timeToMinutes(ledStart);
  int endMin = timeToMinutes(ledEnd);
  if (startMin < 0 || endMin < 0) return false;

  if (startMin <= endMin) {
    // Normal aralık: ör. 08:00 - 20:00
    return (nowMin >= startMin && nowMin < endMin);
  } else {
    // Gece yarısını geçen aralık: ör. 20:00 - 06:00
    return (nowMin >= startMin || nowMin < endMin);
  }
}

// Mevcut RTC saatinin herhangi bir pompa slotunun PUMP_SLOT_DURATION dakikalık
// penceresi içinde olup olmadığını kontrol et
bool isPumpScheduleActive(int nowMin) {
  char slotsCopy[64];
  strncpy(slotsCopy, pumpSlots, sizeof(slotsCopy) - 1);
  slotsCopy[sizeof(slotsCopy) - 1] = '\0';

  char* token = strtok(slotsCopy, ",");
  while (token != NULL) {
    // Boşlukları atla
    while (*token == ' ') token++;

    int slotMin = timeToMinutes(token);
    if (slotMin >= 0) {
      int slotEnd = slotMin + PUMP_SLOT_DURATION;
      if (slotEnd <= 1440) {
        // Normal aralık
        if (nowMin >= slotMin && nowMin < slotEnd) return true;
      } else {
        // Gece yarısını geçen slot (ör. 23:58 + 5dk = 00:03)
        if (nowMin >= slotMin || nowMin < (slotEnd - 1440)) return true;
      }
    }
    token = strtok(NULL, ",");
  }
  return false;
}

// Zamanlama motorunu çalıştır ve röleleri güncelle
void updateRelays() {
  RtcDateTime now = rtc.GetDateTime();
  bool rtcValid = rtc.IsDateTimeValid() && rtc.GetIsRunning() && now.Year() >= 2025;
  int nowMin = rtcValid ? (now.Hour() * 60 + now.Minute()) : -1;

  // === LED Kontrolü ===
  bool newLedStatus;
  if (ledOverride >= 0) {
    // Override aktif: 1=aç, 0=kapat (RTC gerekmez)
    newLedStatus = (ledOverride == 1);
  } else if (rtcValid) {
    // Zamanlama modu (RTC gerekli)
    newLedStatus = isLedScheduleActive(nowMin);
  } else {
    Serial.println("[RELAY] RTC gecersiz - LED son durumda");
    newLedStatus = ledStatus;
  }

  if (newLedStatus != ledStatus) {
    ledStatus = newLedStatus;
    digitalWrite(RELAY_LED_PIN, ledStatus ? LOW : HIGH);
    Serial.printf("[RELAY] LED: %s (%s)\n",
                  ledStatus ? "ACIK" : "KAPALI",
                  ledOverride >= 0 ? "override" : "zamanlama");
  }

  // === Pompa Kontrolü ===
  bool newPumpStatus;
  if (pumpOverride >= 0) {
    // Override aktif: 1=aç, 0=kapat (RTC gerekmez)
    newPumpStatus = (pumpOverride == 1);
  } else if (rtcValid) {
    // Zamanlama modu (RTC gerekli)
    newPumpStatus = isPumpScheduleActive(nowMin);
  } else {
    Serial.println("[RELAY] RTC gecersiz - Pompa son durumda");
    newPumpStatus = pumpStatus;
  }

  if (newPumpStatus != pumpStatus) {
    pumpStatus = newPumpStatus;
    digitalWrite(RELAY_PUMP_PIN, pumpStatus ? LOW : HIGH);
    Serial.printf("[RELAY] Pompa: %s (%s)\n",
                  pumpStatus ? "ACIK" : "KAPALI",
                  pumpOverride >= 0 ? "override" : "zamanlama");
  }
}