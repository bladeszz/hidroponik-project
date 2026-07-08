// ============================================================
//  CONFIG.H - Ayarlar ve Pin Tanımlamaları
//  Tüm sabitler, zamanlama, pin ayarları ve kalibrasyon burada
// ============================================================

#ifndef CONFIG_H
#define CONFIG_H

// ==================== SUPABASE AYARLARI ====================
// Yeni Supabase projesinden alınan bilgiler (Settings > API)
const char* SUPABASE_URL = "https://ppqypkesnvvtvdluatrl.supabase.co";     // ör: https://xxxxx.supabase.co
const char* SUPABASE_KEY = "******";          // ör: eyJhbGci...

// ==================== AUTH HEADER ====================
char SUPABASE_AUTH[300]; // "******" - setup()'da doldurulur

// ==================== ZAMANLAMA AYARLARI ====================
#define SERIAL_PRINT_INTERVAL 2000     // Seri monitör yazdırma (ms)
#define SUPABASE_SEND_INTERVAL 10000   // Veri gönderimi (ms)
#define COMMAND_CHECK_INTERVAL 5000    // Komut kontrolü (ms)
#define TDS_SAMPLE_INTERVAL 40         // TDS örnekleme (ms)
#define AP_TIMEOUT 300                 // WiFi AP süresi (saniye) = 5 dakika
#define WIFI_RECONNECT_INTERVAL 30000  // WiFi yeniden bağlanma kontrolü (ms)
#define WDT_TIMEOUT 60                 // Watchdog süresi (saniye)
#define HTTP_TIMEOUT 10000             // Supabase HTTP timeout (ms)
#define MAX_RECONNECT_ATTEMPTS 3       // WiFi.reconnect() deneme sayısı
#define SCHEDULE_CHECK_INTERVAL 60000  // Zamanlama motoru kontrolü (ms)
#define PUMP_SLOT_DURATION 5           // Pompa çalışma süresi (dakika/slot)

// ==================== PIN TANIMLAMALARI ====================
#define RESET_BTN_PIN 0     // WiFi reset butonu (BOOT)

// TDS Sensörü
#define TDS_SIGNAL_PIN 32
#define TDS_POWER_PIN 16

// DHT22 Sıcaklık/Nem
#define DHT_PIN 4
#define DHT_TYPE DHT22

// Su Seviye Sensörü (HW-038)
#define WATER_POWER_PIN 17
#define WATER_SIGNAL_PIN 36

// RTC Modülü (DS1302)
#define RTC_DAT_PIN 26
#define RTC_CLK_PIN 25
#define RTC_RST_PIN 27

// Röle Pinleri
#define RELAY_PUMP_PIN 5    // Pompa rölesi
#define RELAY_LED_PIN 18    // LED rölesi

// ==================== SABİTLER ====================
#define VREF 3.3
#define TDS_SAMPLE_COUNT 30

// TDS Kalibrasyon Sabitleri (Prob: TDS Meter V1.0)
#define TDS_FACTOR 0.5
#define TDS_COEFF_A 133.42
#define TDS_COEFF_B 255.86
#define TDS_COEFF_C 857.39

#endif