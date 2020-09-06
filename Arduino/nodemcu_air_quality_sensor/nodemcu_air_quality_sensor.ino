#define READ_MODE 0
#define WIFI_SERVER_MODE 1

#define USE_SERIAL 0
#define USE_DISPLAY 1

#if USE_DISPLAY
#include <Adafruit_SSD1306.h>
#endif

#if WIFI_SERVER_MODE
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#endif

// kOutPin is any analog pin connected to pin 1 (OUT) on the sensor.
// A0 (ADC0)
const int kOutPin = A0;
// kDrivePin is any digital pin connected to pin 2 (LED) on the sensor.
// D5 (GPIO14)
const int kLedPin = D5;

#if USE_DISPLAY
// D1 (GPIO5) = SCL
// D2 (GPIO4) = SDA
Adafruit_SSD1306 display(128, 64);
#endif

#if WIFI_SERVER_MODE
// Use your Wi-Fi configuration values.
const char * const ssid = "";
const char * const password = "";

ESP8266WebServer server(80);
#endif

int readOutput() {
  // To drive the sensor LDE, it requires to have a inverter circuit to send pulse to the sensor.
  // Use NPN transistor such as 2SC1815 and connect kLedPin to the base, GND to emittor, and sensor LDE pin to the collector.
  // This is especially needed because sensor VCC requires 5V, not 3.3V used on NodeMCU.

  // See https://github.com/sharpsensoruser/sharp-sensor-demos/wiki/Application-Guide-for-Sharp-GP2Y1014AU0F-Dust-Sensor
  // and related documents for the detailed behaviors.
  digitalWrite(kLedPin, HIGH);

  // Speced wait time to measure is 280us, however, `analogRead()` takes long time.
  // reduce it to target better measuring timing.
  // This 140us is just based on the observation.
  delayMicroseconds(140);

  // This is actually slow, takes 100us to 200us.
  int output = analogRead(kOutPin);

  // Total pulse duration should be ~320us therefore at least wait 40us.
  delayMicroseconds(40);
  digitalWrite(kLedPin, LOW);

  // Minimum pulse interval is 10ms, so wait another 9.68ms.
  delayMicroseconds(9680);

  return output;
}

float readOutputMulti(int count) {
  int results = 0;
  for (int i = 0; i < count; i++) {
    results += readOutput();
    delay(100);
  }
  return float(results) / float(count);
}

void setup() {
  pinMode(kLedPin, OUTPUT);
  pinMode(kOutPin, INPUT);

#if WIFI_SERVER_MODE
  pinMode(LED_BUILTIN, OUTPUT);
#endif

#if USE_SERIAL
  Serial.begin(115200);
  Serial.println();
#endif
#if USE_DISPLAY
  // I2C Address is configured as 0x78 by default and give 2 bit shifted value as 2nd argument.
  // I don't know why, ...yet.
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    exit(0);
  }
#endif

  delay(2000);

#if USE_DISPLAY
  display.setTextSize(1);
  display.setTextColor(WHITE);
#endif

#if WIFI_SERVER_MODE
#if USE_SERIAL
  Serial.print("Connecting");
#endif
#if USE_DISPLAY
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Connecting");
  display.display();
#endif

#if USE_SERIAL
  WiFi.printDiag(Serial);
#endif

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#if USE_SERIAL
    Serial.print(".");
#endif
#if USE_DISPLAY
    display.print(".");
    display.display();
#endif
  }
  digitalWrite(LED_BUILTIN, HIGH);

#if USE_SERIAL
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif
#if USE_DISPLAY
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("IP address: ");
  display.println(WiFi.localIP());
  display.display();
#endif

  server.on("/metrics", []() {
    digitalWrite(LED_BUILTIN, LOW);

    int output = readOutputMulti(20);
    float voltage = float(output) * (5.0 / 1024.0);

    // This is Prometheus metrics format.
    // Voltage is an optional metric, which you can also calcurate on Prometheus query.
    String body =
      "#HELP nodemcu_ks0196_output output\n"
      "#TYPE nodemcu_ks0196_output gauge\n"
      "nodemcu_ks0196_output " + String(output) + "\n"
      "#HELP nodemcu_ks0196_voltage voltage\n"
      "#TYPE nodemcu_ks0196_voltage gauge\n"
      "nodemcu_ks0196_voltage " + String(voltage) + "\n";
    server.send(200, "text/plain", body);

#if USE_SERIAL
    Serial.print(output);
    Serial.print("\t");
    Serial.print(voltage);
    Serial.print("\n");
#endif
#if USE_DISPLAY
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("IP address: ");
    display.println(WiFi.localIP());
    display.print("output: ");
    display.println(output);
    display.print("voltage: ");
    display.println(voltage);
    display.display();
#endif

    digitalWrite(LED_BUILTIN, HIGH);
  });

  server.begin();
#endif
}

void loop() {
#if WIFI_SERVER_MODE
  server.handleClient();
#endif

#if READ_MODE
  delay(1000);

  float output = readOutputMulti(20);
  float voltage = output * (5.0 / 1024.0);

#if USE_SERIAL
  Serial.print(output);
  Serial.print("\t");
  Serial.print(voltage);
  Serial.print("\n");
#endif
#if USE_DISPLAY
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("output: ");
  display.println(output);
  display.print("voltage: ");
  display.println(voltage);
  display.display();
#endif
#endif
}
