#include "myheaders.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void displaySetup() {
  uint32_t start_time = millis();
  Serial.println("Setting up display");
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.setTextColor(WHITE);
  Serial.println("Display initializing... give me some time"); //does need 1-2 secs
  Serial.print("Display startup took ");
  Serial.println(millis() - start_time);
}

void displayRedraw(char temp[],char temp_yesterday[], char status[]) {
  uint32_t start_time = millis();
  display.clearDisplay();
  
  // display temperature
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Pufferspeicher-Temp: ");
  display.setTextSize(3);
  display.setCursor(0,20);
  display.print(temp);
  display.setTextSize(2);
  display.cp437(true);
  display.write(248);
  display.setTextSize(2);
  display.print("C");
  
  // display temperature 24h ago
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.print("Yesterday: ");
  display.setCursor(65, 50);
  display.print(temp_yesterday);
  display.write(248);
  display.print("C");
  display.display(); 
  Serial.print("Print on display took ");
  Serial.println(millis() - start_time);
}

