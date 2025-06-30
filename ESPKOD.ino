#define BLYNK_TEMPLATE_ID "TMPL6KwO_wXtV"
#define BLYNK_TEMPLATE_NAME "Smart Home"
#define BLYNK_AUTH_TOKEN "jrmii4-X8SKJO0j9Tw3unya-h6dT11l4"

#include <Wire.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "ssidkey";
char pass[] = "password";

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  Blynk.run();
}

void sendCommand(char komut) {
  Wire.beginTransmission(0x42);
  Wire.write(komut);
  Wire.endTransmission();
  Serial.print("Gönderilen komut: ");
  Serial.println(komut);
}


BLYNK_WRITE(V0) { sendCommand(param.asInt() == 1 ? 'K' : 'k'); } // Kapı
BLYNK_WRITE(V1) { sendCommand(param.asInt() == 1 ? 'G' : 'g'); } // Garaj
BLYNK_WRITE(V2) { sendCommand(param.asInt() == 1 ? 'I' : 'i'); } // Işık
BLYNK_WRITE(V3) { sendCommand(param.asInt() == 1 ? 'F' : 'f'); } // Fan

