// ============================================================
//  WIFI_UTILS.INO - WiFi Yardımcı Fonksiyonları
//  Cihaz ID ve MAC Adresi oluşturma
// ============================================================

// Device ID oluştur (MAC son 6 hane → global char deviceID[7])
void initDeviceID() {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  snprintf(deviceID, sizeof(deviceID), "%02X%02X%02X", mac[3], mac[4], mac[5]);
}

// Tam MAC Adresini yazdır (heap alloc yok, direkt Serial'e)
void printMAC() {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  Serial.printf("[INFO] MAC Adres: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}