#include <M5StickCPlus.h>
#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>

// WiFi Parameters
const char* ssid = "Varun";
const char* password = "12345678";

float cTemp    = 0;
float fTemp    = 0;
float humidity = 0;
char heatingMode = 'H';
float tempThreshold = 28;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  M5.Lcd.println();
  M5.Lcd.print("Connecting to ");
  M5.Lcd.println(ssid);

  //WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }
  M5.Lcd.println("");
  M5.Lcd.println("WiFi connected");
  M5.Lcd.println("IP address: ");
  M5.Lcd.println(WiFi.localIP());
}

void setup() {
  M5.begin();
  setup_wifi();
  Wire.begin(0,26);
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 4);
}

void loop() {
  unsigned int data[6];

  // Start I2C Transmission
  Wire.beginTransmission(0x44);
  // Send measurement command
  Wire.write(0x2C);
  Wire.write(0x06);
  // Stop I2C transmission
  if (Wire.endTransmission() != 0){
    Serial.println("ERROR -- endTransmission Error!");
  }
  else{
    delay(500);
    // Request 6 bytes of data
    Wire.requestFrom(0x44, 6);

    // Read 6 bytes of data
    // cTemp msb, cTemp lsb, cTemp crc, humidity msb, humidity lsb, humidity crc
    for (int i = 0; i < 6; i++) {
      data[i] = Wire.read();
    };
    delay(50);

    if (Wire.available() != 0){
      Serial.println("ERROR -- Wire.available still has data!");
    }
    else{
      // Convert the data

      M5.Lcd.fillScreen(TFT_GREEN);
      M5.Lcd.setTextColor(TFT_BLACK); 

      cTemp    = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
      fTemp    = (cTemp * 1.8) + 32;
      humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);
      Serial.print("celsius: ");
      Serial.println(cTemp);
      M5.Lcd.setCursor(0, 0, 2);
      // M5.Lcd.printf("Temp: %2.1f °C Humi: %2.0f", cTemp, humidity);
      M5.Lcd.printf("Temp: %2.1f °C | Threshold: %2.1f °C", cTemp, tempThreshold);

      M5.update();

      if (M5.BtnA.isPressed()) { 
        tempThreshold+=1;
        M5.Lcd.printf("\nThreshold increased to %2.1f °C", tempThreshold);
      } else if (M5.BtnB.isPressed()) {
        tempThreshold-=1;
        M5.Lcd.printf("\nThreshold decreased to %2.1f °C", tempThreshold);
      }

      if (cTemp > tempThreshold && heatingMode == 'H') {
        M5.Lcd.fillScreen(TFT_RED);
        M5.Lcd.printf("\n%2.1f °C > %2.1f °C.\nSwitching to cooling mode.\n", cTemp, tempThreshold);
        heatingMode = 'C';

        //IFTT - Slack Post Request
        HTTPClient http;
        http.begin("https://maker.ifttt.com/trigger/temp_cooler/json/with/key/bHzi9d57afpvYlE9dgfAMlHBE0qQzWqmB6o4eew2Umi");
        http.GET();
        http.end();

      } else if (cTemp > tempThreshold && heatingMode == 'C') {
        M5.Lcd.printf("\n%2.1f °C > %2.1f °C.\nStay in cooling mode.\n", cTemp, tempThreshold);
      } else if (cTemp <= tempThreshold && heatingMode == 'H') {
        M5.Lcd.printf("\n%2.1f °C <= %2.1f °C.\nStay in heating mode.\n", cTemp, tempThreshold);
      } else {
        M5.Lcd.fillScreen(TFT_BLUE);
        M5.Lcd.printf("\n%2.1f °C <= %2.1f °C.\nSwitching to heating mode.\n", cTemp, tempThreshold);
        heatingMode = 'H';

        //IFTT - Slack Post Request
        HTTPClient http;
        http.begin("https://maker.ifttt.com/trigger/temp_heater/json/with/key/bHzi9d57afpvYlE9dgfAMlHBE0qQzWqmB6o4eew2Umi");
        http.GET();
        http.end();
        
      }
    }
  }
  delay(3000);
  M5.Lcd.fillScreen(BLACK);
}