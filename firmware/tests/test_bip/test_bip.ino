// Test DAC minimal : son pur 440 Hz en I2S vers le PCM5102A, sans SD ni decodeur MP3
// Sert a verifier le cablage du DAC seul. Alterne toutes les 2 s : 440 Hz, puis 880 Hz (plus aigu)
// Librairie ESP_I2S incluse dans le core esp32 3.x

#include <ESP_I2S.h>

#define I2S_BCK   7
#define I2S_DIN   6
#define I2S_LRCK  5

#define FREQ_ECH  44100
#define AMPLITUDE 8000   // sur 32767 : environ 1/4 du maximum

I2SClass i2s;
int16_t tampon[2 * 256];   // 256 echantillons stereo (gauche, droite)
float phase = 0;

void log(const String &txt) {
  Serial.println(txt);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial0.println(txt);
#endif
}

void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial0.begin(115200);
#endif
  delay(1500);
  log("\n=== Test bip I2S ===");
  i2s.setPins(I2S_BCK, I2S_LRCK, I2S_DIN);
  if (!i2s.begin(I2S_MODE_STD, FREQ_ECH, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
    log("I2S : ECHEC initialisation");
    while (true) delay(1000);
  }
  log("I2S OK : BCK=7 LRCK=5 DIN=6, 44,1 kHz 16 bits stereo");
}

uint32_t dernierLog = 0;

void loop() {
  bool aigu = (millis() / 2000) % 2;
  float freq = aigu ? 880 : 440;
  for (int i = 0; i < 256; i++) {
    int16_t v = (int16_t)(AMPLITUDE * sinf(phase));
    tampon[2 * i] = v;       // gauche
    tampon[2 * i + 1] = v;   // droite
    phase += 2 * PI * freq / FREQ_ECH;
    if (phase > 2 * PI) phase -= 2 * PI;
  }
  i2s.write((uint8_t *)tampon, sizeof(tampon));
  if (millis() - dernierLog > 2000) {
    dernierLog = millis();
    log(String("Bip ") + (int)freq + " Hz");
  }
}
