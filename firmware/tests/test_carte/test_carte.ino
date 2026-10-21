// Test de la carte seule (sans écran ni SD) : puce, flash, PSRAM, broches boutons
#include <esp_chip_info.h>
#include <esp_flash.h>

const int BOUTONS[] = {42, 41, 40, 39, 38, 1};
const char *NOMS[]  = {"MENU", "PLAY", "PREV", "NEXT", "VOL+", "VOL-"};

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== Test carte ESP32-S3 ===");

  esp_chip_info_t info;
  esp_chip_info(&info);
  Serial.printf("Puce      : %s rev %d, %d coeurs\n", ESP.getChipModel(), info.revision, info.cores);
  Serial.printf("CPU       : %lu MHz\n", ESP.getCpuFreqMHz());

  uint32_t flash = 0;
  esp_flash_get_size(NULL, &flash);
  Serial.printf("Flash     : %lu Mo\n", flash / (1024 * 1024));

  if (psramFound()) {
    Serial.printf("PSRAM     : %lu Ko (libre %lu Ko)\n", ESP.getPsramSize() / 1024, ESP.getFreePsram() / 1024);
    // Écriture/relecture de 1 Mo en PSRAM
    const size_t n = 1024 * 1024;
    uint32_t *buf = (uint32_t *)ps_malloc(n);
    bool ok = buf != nullptr;
    if (ok) {
      for (size_t i = 0; i < n / 4; i++) buf[i] = i * 2654435761u;
      for (size_t i = 0; i < n / 4 && ok; i++) ok = buf[i] == i * 2654435761u;
      free(buf);
    }
    Serial.printf("Test PSRAM: %s\n", ok ? "OK" : "ECHEC");
  } else {
    Serial.println("PSRAM     : NON DETECTEE");
  }
  Serial.printf("Heap libre: %lu Ko\n", ESP.getFreeHeap() / 1024);

  for (int i = 0; i < 6; i++) pinMode(BOUTONS[i], INPUT_PULLUP);
  Serial.println("Boutons (1 = relâché, 0 = appuyé / relié à GND) :");
}

void loop() {
  for (int i = 0; i < 6; i++) Serial.printf("%s=%d ", NOMS[i], digitalRead(BOUTONS[i]));
  Serial.println();
  delay(2000);
}
