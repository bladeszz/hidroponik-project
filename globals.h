// ============================================================
//  GLOBALS.H - Global Değişkenler ve Nesneler
//  Tüm sensör değişkenleri, durum bayrakları ve nesneler burada
// ============================================================

#ifndef GLOBALS_H
#define GLOBALS_H

// ==================== GLOBAL DEĞİŞKENLER ====================
// Cihaz Kimliği (MAC son 6 hex = 6 karakter + null)
char deviceID[7] = "";

// TDS değişkenleri
int tdsBuffer[TDS_SAMPLE_COUNT];
int tdsBufferIndex = 0;
float tdsValue = 0;

// Sensör değişkenleri (NAN = henüz okunmadı, -1 gönderilir)
float temperature = NAN;
float humidity = NAN;
int waterLevel = 0;
bool readingWaterLevel = false;

// Röle durumları
bool pumpStatus = false;
bool ledStatus = false;

// Zamanlama değişkenleri (Supabase'den okunur)
char ledStart[6] = "08:00";       // LED açılma saati (HH:MM)
char ledEnd[6] = "20:00";         // LED kapanma saati (HH:MM)
int8_t ledOverride = -1;          // -1=zamanlama, 0=zorla kapat, 1=zorla aç
char pumpSlots[64] = "08:00,20:00"; // Pompa çalışma saatleri (virgülle ayrılmış)
int8_t pumpOverride = -1;         // -1=zamanlama, 0=zorla kapat, 1=zorla aç
int lastResetDay = -1;            // Override gece yarısı sıfırlama için son reset günü

// Sensör hata durumları
bool dhtError = true;  // Başlangıçta true, ilk başarılı okumada false

// WiFi bağlantı durumu
bool wasConnected = false;
int wifiReconnectAttempts = 0;

// WiFi Portal durumu
bool portalActive = false;
unsigned long portalStartTime = 0;
bool deviceRegistered = false;

// Sensör nesneleri
DHT dht(DHT_PIN, DHT_TYPE);
ThreeWire rtcWire(RTC_DAT_PIN, RTC_CLK_PIN, RTC_RST_PIN);
RtcDS1302<ThreeWire> rtc(rtcWire);

// WiFiManager
WiFiManager wifiManager;

#endif