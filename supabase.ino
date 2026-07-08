// ============================================================
//  SUPABASE.INO - Supabase İletişim Fonksiyonları
//  Komut okuma, cihaz kayıt, veri gönderimi
// ============================================================

// Ortak HTTP header'ları ekle
void addSupabaseHeaders(HTTPClient &http, bool withContent) {
  http.setTimeout(HTTP_TIMEOUT);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", SUPABASE_AUTH);
  if (withContent) {
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Prefer", "return=minimal");
  }
}

// Supabase'den Zamanlama Ayarlarını Oku
void checkCommands() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  char url[256];
  snprintf(url, sizeof(url), "%s/rest/v1/devices?device_id=eq.%s&select=led_start,led_end,led_override,pump_slots,pump_override",
           SUPABASE_URL, deviceID);

  http.begin(url);
  addSupabaseHeaders(http, false);

  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    if (!deserializeJson(doc, payload) && doc.size() > 0) {
      // LED zamanlama
      const char* ls = doc[0]["led_start"] | "08:00";
      const char* le = doc[0]["led_end"] | "20:00";
      strncpy(ledStart, ls, sizeof(ledStart) - 1);
      ledStart[sizeof(ledStart) - 1] = '\0';
      strncpy(ledEnd, le, sizeof(ledEnd) - 1);
      ledEnd[sizeof(ledEnd) - 1] = '\0';

      // LED override (null=-1, true=1, false=0)
      if (doc[0]["led_override"].isNull()) {
        ledOverride = -1;
      } else {
        ledOverride = doc[0]["led_override"].as<bool>() ? 1 : 0;
      }

      // Pompa slotları
      const char* ps = doc[0]["pump_slots"] | "08:00,20:00";
      strncpy(pumpSlots, ps, sizeof(pumpSlots) - 1);
      pumpSlots[sizeof(pumpSlots) - 1] = '\0';

      // Pompa override (null=-1, true=1, false=0)
      if (doc[0]["pump_override"].isNull()) {
        pumpOverride = -1;
      } else {
        pumpOverride = doc[0]["pump_override"].as<bool>() ? 1 : 0;
      }

      // Override veya zamanlama değişikliğini hemen uygula
      updateRelays();
    }
  } else if (code < 0) {
    Serial.printf("[HATA] Zamanlama okuma: %s\n", http.errorToString(code).c_str());
  } else {
    Serial.printf("[UYARI] Zamanlama HTTP: %d\n", code);
  }
  http.end();
}

// Cihazı Supabase'e KayDet/Güncelle — başarılıysa true döner
bool registerDevice() {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  char url[256];
  char json[256];

  // Cihaz var mı kontrol et
  snprintf(url, sizeof(url), "%s/rest/v1/devices?device_id=eq.%s&select=device_id",
           SUPABASE_URL, deviceID);
  http.begin(url);
  addSupabaseHeaders(http, false);

  int code = http.GET();
  bool deviceExists = false;
  if (code == 200) {
    Stream& stream = http.getStream();
    StaticJsonDocument<256> doc;
    deserializeJson(doc, stream);
    deviceExists = (doc.is<JsonArray>() && doc.size() > 0);
  }
  http.end();
  Serial.printf("[DB] Cihaz kontrol: %d - %s\n", code, deviceExists ? "MEVCUT" : "YENi");

  if (!deviceExists) {
    // Yeni cihaz - INSERT
    Serial.println("[DB] Yeni cihaz kaydediliyor...");
    snprintf(url, sizeof(url), "%s/rest/v1/devices", SUPABASE_URL);
    snprintf(json, sizeof(json),
      "{\"device_id\":\"%s\",\"power_status\":true,\"wifi_status\":true}",
      deviceID);

    http.begin(url);
    addSupabaseHeaders(http, true);

    code = http.POST(json);
    http.end();

    if (code == 201) {
      Serial.println("[DB] Kayit: OK");
      return true;
    }
    Serial.printf("[DB] Kayit HATA: %d\n", code);
    return false;
  } else {
    // Mevcut cihaz - durum güncelle
    Serial.println("[DB] Cihaz mevcut - durum guncelleniyor");
    snprintf(url, sizeof(url), "%s/rest/v1/devices?device_id=eq.%s",
             SUPABASE_URL, deviceID);
    snprintf(json, sizeof(json),
      "{\"power_status\":true,\"wifi_status\":true}");

    http.begin(url);
    addSupabaseHeaders(http, true);

    code = http.PATCH(json);
    http.end();

    if (code == 204) {
      Serial.println("[DB] Guncelleme: OK");
      return true;
    }
    Serial.printf("[DB] Guncelleme HATA: %d\n", code);
    return false;
  }
}

// Supabase'e Veri Gönder
void sendToSupabase() {
  if (WiFi.status() != WL_CONNECTED) return;

  // RTC kontrolü
  RtcDateTime now = rtc.GetDateTime();
  char timestamp[25];
  if (rtc.IsDateTimeValid() && rtc.GetIsRunning() && now.Year() >= 2025) {
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02d",
             now.Year(), now.Month(), now.Day(),
             now.Hour(), now.Minute(), now.Second());
  } else {
    snprintf(timestamp, sizeof(timestamp), "1970-01-01T00:00:00");
    Serial.println("[UYARI] RTC gecersiz! Varsayilan tarih kullaniliyor");
  }

  // JSON oluştur (DHT hatası → -1 gönderilir)
  char json[512];
  snprintf(json, sizeof(json),
    "{\"temperature\":%.1f,\"humidity\":%.1f,\"water_level\":%d,\"tds\":%.0f,"
    "\"clock_time\":\"%s\",\"last_seen\":\"now()\",\"power_status\":true,\"wifi_status\":true,"
    "\"pump_status\":%s,\"led_status\":%s}",
    dhtError ? -1.0 : temperature,
    dhtError ? -1.0 : humidity,
    waterLevel, tdsValue, timestamp,
    pumpStatus ? "true" : "false",
    ledStatus ? "true" : "false");

  char url[256];
  snprintf(url, sizeof(url), "%s/rest/v1/devices?device_id=eq.%s",
           SUPABASE_URL, deviceID);

  HTTPClient http;
  http.begin(url);
  addSupabaseHeaders(http, true);

  int code = http.PATCH(json);
  if (code == 204) {
    Serial.println("Supabase: OK");
  } else {
    Serial.printf("Supabase HATA: %d\n", code);
  }
  http.end();
}