// ============================================================
//  TOPRAKSIZ TARIM - HİDROPONİK SİSTEM (TİCARİ VERSİYON)
//  Versiyon: 3.2 - Zamanlama Motorlu Sürüm
//  Tarih: Şubat 2026
// ============================================================
//  Sensörler: TDS, DHT22, Su Seviye (HW-038), RTC (DS1302)
//  Kontrol: Röle (Pompa, LED) - Zamanlama + Override
//  Bağlantı: WiFiManager (Captive Portal) + Otomatik Yeniden Bağlanma
//  Veri: Supabase (device_id bazlı)
//  Güvenlik: Watchdog Timer, Sensör Hata Yönetimi, Bellek Optimizasyonu
// ============================================================
//
//  DOSYA YAPISI:
//  Hidroponik.ino   → Ana dosya (setup + loop)
//  config.h          → Ayarlar, pin tanımları, sabitler
//  globals.h         → Global değişkenler, nesneler
//  sensors.ino       → TDS, su seviye, medyan hesaplama
//  relays.ino        → Röle kontrol fonksiyonu
//  supabase.ino      → Supabase iletişim (komut, kayıt, veri)
//  wifi_utils.ino    → Cihaz ID, MAC adresi
//
// ============================================================

#include "DHT.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <ArduinoJson.h>
#include "esp_mac.h"
#include <esp_task_wdt.h>

#include "config.h"
#include "globals.h"

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n============================================");
  Serial.println("  TOPRAKSIZ TARIM - Hidroponik Sistem v3.2");
  Serial.println("============================================\n");

  // Auth header oluştur
  snprintf(SUPABASE_AUTH, sizeof(SUPABASE_AUTH), "******", SUPABASE_KEY);

  // Device ID ve MAC oluştur
  WiFi.mode(WIFI_STA);
  initDeviceID();

  Serial.println("-------- CIHAZ BILGILERI --------");
  Serial.printf("[INFO] Cihaz ID : %s\n", deviceID);
  printMAC();
  Serial.println("---------------------------------");

  // Ortamdaki WiFi ağlarını tara ve göster
  Serial.println("\n[WIFI] Ortamdaki aglar taraniyor...");
  int n = WiFi.scanNetworks();
  if (n > 0) {
    Serial.printf("[WIFI] %d ag bulundu:\n", n);
    for (int i = 0; i < n; i++) {
      Serial.printf("  %d) %s (%d dBm) %s\n",
                    i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i),
                    WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "[ACIK]" : "");
    }
  } else {
    Serial.println("[WIFI] Hic ag bulunamadi!");
  }
  WiFi.scanDelete();

  // BOOT butonu (GPIO 0) - loop'ta 3sn basılı tutunca WiFi sıfırlar
  pinMode(RESET_BTN_PIN, INPUT_PULLUP);

  // WiFiManager ayarları
  char apName[24];
  snprintf(apName, sizeof(apName), "Hidroponik-%s", deviceID);

  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setConfigPortalTimeout(0);  // Timeout'u biz yönetiyoruz
  wifiManager.setConnectTimeout(15);

  // Portal görünümü
  wifiManager.setTitle("Hidroponik Sistem");
  wifiManager.setDarkMode(true);
  std::vector<const char*> menu = {"wifi", "exit"};
  wifiManager.setMenu(menu);

  wifiManager.setCustomHeadElement(
    "<style>"
    "body{font-family:'Segoe UI',Arial,sans-serif;}"
    ".wrap{max-width:380px;padding:0 16px;}"
    "h1{font-size:1.6em;text-align:center;margin:16px 0 8px;color:#2ecc71;}"
    "h3{text-align:center;font-weight:400;color:#aaa;margin:0 0 16px;font-size:0.85em;}"
    ".msg{background:#1a1a2e;border-left:4px solid #2ecc71;padding:12px 16px;"
    "border-radius:0 8px 8px 0;margin:12px 0;font-size:0.9em;}"
    ".msg b{color:#2ecc71;}"
    "button,input[type='submit']{background:#2ecc71!important;border:none!important;"
    "border-radius:20px!important;padding:12px!important;font-size:1em!important;"
    "font-weight:600!important;letter-spacing:0.5px;cursor:pointer;"
    "transition:background 0.3s!important;}"
    "button:hover,input[type='submit']:hover{background:#27ae60!important;}"
    "input[type='text'],input[type='password']{border:2px solid #333!important;"
    "border-radius:10px!important;padding:10px 14px!important;font-size:0.95em!important;"
    "background:#1a1a2e!important;color:#eee!important;}"
    "input[type='text']:focus,input[type='password']:focus{border-color:#2ecc71!important;"
    "outline:none!important;}"
    "a{color:#2ecc71!important;text-decoration:none!important;}"
    "#wifi_scan{border-radius:12px;overflow:hidden;}"
    ".q{color:#2ecc71!important;}"
    "</style>"
  );

  // Bilgi kartı (static = setup sonrasında da bellekte kalır → crash önlenir)
  static char apMessage[256];
  String ssid = WiFi.SSID();  // Geçici String'i yakala (dangling pointer önlenir)
  if (ssid.length() > 0) {
    snprintf(apMessage, sizeof(apMessage),
      "<div class='msg'>"
      "&#127793; <b>Hidroponik Sistem</b><br>"
      "&#128246; Bagli ag: <b>%s</b>"
      "</div>",
      ssid.c_str());
  } else {
    snprintf(apMessage, sizeof(apMessage),
      "<div class='msg'>"
      "&#127793; <b>Hidroponik Sistem</b><br>"
      "&#128246; WiFi aginizi secin ve sifreyi girin."
      "</div>");
  }
  static WiFiManagerParameter custom_text(apMessage);
  wifiManager.addParameter(&custom_text);

  // Kayıtlı WiFi'ye bağlanmayı dene
  Serial.println("\n[WIFI] Kayitli WiFi kontrol ediliyor...");
  WiFi.begin();
  unsigned long connectStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - connectStart < 10000) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] Kayitli ag bulundu!");
    Serial.printf("[WIFI] Ag: %s | IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    wasConnected = true;
  } else {
    Serial.println("[WIFI] Kayitli ag yok - portalden ayarlayin");
  }

  // WiFi ayarları
  // Portal aktifken ESP32 auto-reconnect kullanılır
  // Portal kapanınca Section 5 devralır (çakışma önlenir)
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);  // Her WiFi.begin()'de NVS yazmasını önle (flash aşınması)

  // Config portal başlat (STA+AP birlikte çalışır)
  wifiManager.startConfigPortal(apName, "topraksiz");
  portalActive = true;
  portalStartTime = millis();
  Serial.printf("[WIFI] Ayar portali acik (%d dk) - Ag: %s\n", AP_TIMEOUT / 60, apName);
  Serial.println("[WIFI] Portal: http://192.168.4.1");

  // RTC Başlat
  rtc.Begin();
  if (rtc.IsDateTimeValid() && rtc.GetIsRunning()) {
    Serial.println("[OK] RTC hazir");
  } else {
    Serial.println("[UYARI] RTC gecersiz veya durmus!");
  }

  // Pin Ayarları - Sensörler
  pinMode(TDS_SIGNAL_PIN, INPUT);
  pinMode(TDS_POWER_PIN, OUTPUT);
  digitalWrite(TDS_POWER_PIN, HIGH);

  pinMode(WATER_POWER_PIN, OUTPUT);
  digitalWrite(WATER_POWER_PIN, LOW);

  // Pin Ayarları - Röleler (aktif LOW)
  pinMode(RELAY_PUMP_PIN, OUTPUT);
  pinMode(RELAY_LED_PIN, OUTPUT);
  digitalWrite(RELAY_PUMP_PIN, HIGH);
  digitalWrite(RELAY_LED_PIN, HIGH);

  // Onboarding LED Flash (3x yanıp söner — ilk kurulum göstergesi)
  Serial.println("[LED] Onboarding flash...");
  for (int i = 0; i < 3; i++) {
    digitalWrite(RELAY_LED_PIN, LOW);   // LED AÇ (aktif LOW)
    delay(300);
    digitalWrite(RELAY_LED_PIN, HIGH);  // LED KAPAT
    delay(300);
  }
  Serial.println("[LED] Onboarding tamamlandi");

  // Sensör Başlatma
  dht.begin();
  analogSetAttenuation(ADC_11db);
  Serial.println("[OK] Sensorler hazir");
  Serial.println("[OK] Roleler hazir");

  // Watchdog Timer
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT * 1000,
    .idle_core_mask = 0,
    .trigger_panic = true
  };
  esp_task_wdt_reconfigure(&wdt_config);
  esp_err_t wdt_err = esp_task_wdt_add(NULL);
  if (wdt_err == ESP_OK || wdt_err == ESP_ERR_INVALID_ARG) {
    Serial.printf("[OK] Watchdog aktif (%d sn)\n", WDT_TIMEOUT);
  } else {
    Serial.printf("[UYARI] Watchdog eklenemedi: %d\n", wdt_err);
  }

  Serial.println("\n========== SISTEM HAZIR ==========");
}

// ==================== LOOP ====================
void loop() {
  esp_task_wdt_reset();

  // 1. TDS Örnekleme (Her 40ms)
  static unsigned long tdsSampleTime = millis();
  if (millis() - tdsSampleTime > TDS_SAMPLE_INTERVAL && !readingWaterLevel) {
    tdsSampleTime = millis();
    tdsBuffer[tdsBufferIndex] = analogRead(TDS_SIGNAL_PIN);
    tdsBufferIndex = (tdsBufferIndex + 1) % TDS_SAMPLE_COUNT;
  }

  // 2. Sensör Okuma + Seri Monitör (Her 2 saniye)
  static unsigned long printTime = millis();
  if (millis() - printTime > SERIAL_PRINT_INTERVAL) {
    printTime = millis();

    // Tarih/Saat
    RtcDateTime now = rtc.GetDateTime();
    Serial.printf("Tarih: %02d/%02d/%04d  Saat: %02d:%02d:%02d\n",
                  now.Day(), now.Month(), now.Year(),
                  now.Hour(), now.Minute(), now.Second());

    // Sıcaklık/Nem
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      dhtError = true;
      Serial.println("[HATA] DHT okunamiyor! Son bilinen deger kullaniliyor");
    } else {
      dhtError = false;
      temperature = t;
      humidity = h;
      Serial.printf("Nem: %.1f%%  Sicaklik: %.1f C\n", humidity, temperature);
    }

    // Su Seviyesi
    waterLevel = readWaterLevel();
    Serial.printf("Su Seviye: %d\n", waterLevel);

    // TDS
    tdsValue = calculateTDS();
    Serial.printf("TDS: %.0f ppm\n", tdsValue);

    // Durum Özeti
    Serial.printf("Pompa: %s  LED: %s", pumpStatus ? "ACIK" : "KAPALI", ledStatus ? "ACIK" : "KAPALI");
    if (dhtError) Serial.print("  [!DHT]");
    Serial.printf("  WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "OK" : "YOK");
    Serial.println("-----------------------");
  }

  // 3. Zamanlama Ayarlarını Oku (Her 5 saniye)
  static unsigned long commandTime = millis();
  if (millis() - commandTime > COMMAND_CHECK_INTERVAL) {
    commandTime = millis();
    checkCommands();
  }

  // 4. Supabase Gönderimi (Her 10 saniye)
  static unsigned long supabaseTime = millis();
  if (millis() - supabaseTime > SUPABASE_SEND_INTERVAL) {
    supabaseTime = millis();
    sendToSupabase();
  }

  // 5. Zamanlama Motoru (Her 60 saniye — röleleri RTC saatine göre güncelle)
  static unsigned long scheduleTime = millis();
  if (millis() - scheduleTime > SCHEDULE_CHECK_INTERVAL) {
    scheduleTime = millis();
    updateRelays();

    // Gece yarısı override sıfırlama (00:00-00:01 arası, günde 1 kez)
    RtcDateTime now = rtc.GetDateTime();
    if (rtc.IsDateTimeValid() && rtc.GetIsRunning() && now.Year() >= 2025) {
      int today = now.Day();
      if (now.Hour() == 0 && now.Minute() == 0 && lastResetDay != today) {
        lastResetDay = today;
        if (ledOverride >= 0 || pumpOverride >= 0) {
          ledOverride = -1;
          pumpOverride = -1;
          Serial.println("[SCHEDULE] Gece yarisi: Override sifirlandi → zamanlama aktif");
        }
      }
    }
  }

  // 6. WiFi Yeniden Bağlanma (Her 30 saniye — portal aktifken devre dışı)
  static unsigned long wifiCheckTime = millis();
  if (!portalActive && millis() - wifiCheckTime > WIFI_RECONNECT_INTERVAL) {
    wifiCheckTime = millis();

    if (WiFi.status() != WL_CONNECTED) {
      if (wasConnected) {
        Serial.println("[WIFI] Baglanti koptu!");
        Serial.printf("[OFFLINE] Roleler son durumda - Pompa: %s, LED: %s\n",
                      pumpStatus ? "ACIK" : "KAPALI", ledStatus ? "ACIK" : "KAPALI");
        wasConnected = false;
        wifiReconnectAttempts = 0;
      }

      wifiReconnectAttempts++;
      Serial.printf("[WIFI] Yeniden baglanti denemesi #%d...\n", wifiReconnectAttempts);

      // Non-blocking: WiFi.begin() çağır, sonucu sonraki tur kontrol eder
      WiFi.disconnect(false, false);  // WiFi açık + credential saklı
      delay(200);
      WiFi.begin();

      if (wifiReconnectAttempts > MAX_RECONNECT_ATTEMPTS) {
        Serial.println("[WIFI] Baglanti basarisiz - denemeye devam ediliyor");
        wifiReconnectAttempts = 0;
      }
    } else {
      // Bağlı durumda
      if (!wasConnected) {
        Serial.println("[WIFI] Baglanti kuruldu!");
        Serial.printf("[WIFI] Ag: %s | IP: %s | Sinyal: %d dBm\n",
                     WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
        wasConnected = true;
        wifiReconnectAttempts = 0;

        // WiFi bağlandığında cihaz kaydı yap
        if (!deviceRegistered) {
          deviceRegistered = registerDevice();
        }
      }
    }
  }

  // 7. WiFi Portal Kontrolü (5dk sonra kapat)
  if (portalActive) {
    wifiManager.process();

    // Portal üzerinden WiFi ayarlandıysa
    if (WiFi.status() == WL_CONNECTED && !deviceRegistered) {
      Serial.println("[WIFI] Portal uzerinden baglanti kuruldu!");
      Serial.printf("[WIFI] Ag: %s | IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
      wasConnected = true;
      deviceRegistered = registerDevice();
    }

    if (millis() - portalStartTime > (unsigned long)AP_TIMEOUT * 1000) {
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      WiFi.setAutoReconnect(false);  // Section 6 devralıyor, çakışma önlenir
      portalActive = false;
      Serial.println("[WIFI] Ayar portali kapatildi (5dk doldu)");

      // Portal kapandı - kayıtlı WiFi'ye bağlan
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI] Kayitli WiFi'ye baglaniliyor...");
        WiFi.begin();
      }

      if (WiFi.status() == WL_CONNECTED && !deviceRegistered) {
        deviceRegistered = registerDevice();
      }
    }
  }

  // 8. BOOT Butonu: 3sn basılı tut = WiFi sıfırla + restart
  static unsigned long btnPressStart = 0;
  static bool btnPressed = false;
  if (digitalRead(RESET_BTN_PIN) == LOW) {
    if (!btnPressed) {
      btnPressed = true;
      btnPressStart = millis();
      Serial.println("[BTN] BOOT butonu algilandi - 3sn basili tutun...");
    } else if (millis() - btnPressStart >= 3000) {
      Serial.println("[BTN] WiFi ayarlari sifirlandi! Yeniden baslatiliyor...");
      wifiManager.resetSettings();
      delay(1000);
      ESP.restart();
    }
  } else {
    if (btnPressed && millis() - btnPressStart < 3000) {
      Serial.println("[BTN] Buton erken birakildi");
    }
    btnPressed = false;
  }
}
