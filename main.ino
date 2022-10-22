#include <M5Stack.h>
#include "Adafruit_SGP30.h"

#define CONSOLE Serial

long CONSOL_BAND = 115200;
Adafruit_SGP30 sgp;

void setup()
{
  M5.begin();
  CONSOLE.begin(CONSOL_BAND);
  CONSOLE.println("Setup start");
  if (!sgp.begin())
  {
    CONSOLE.println("Sensor not found :(");
    while (1)
      ;
  }
  M5.Power.begin();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextFont(7);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setCursor(20, 40);
  for(int i = 0; i < 15; i++){
    M5.Lcd.printf(".");
    delay(1000);
  }

  CONSOLE.println("Setup finish");
}

void loop()
{
  M5.Lcd.fillScreen(BLACK);
  if (!sgp.IAQmeasure())
  {
    CONSOLE.println("Measurement failed");
    return;
  }
  CONSOLE.print("TVOC ");
  CONSOLE.print(sgp.TVOC);
  CONSOLE.print(" ppb\t");
  CONSOLE.print("eCO2 ");
  CONSOLE.print(sgp.eCO2);
  CONSOLE.println(" ppm");
  drawPower(0, 0);
  drawTVOC(0, 60);
  drawECO2(160, 60);
  delay(1000);
}

static const int32_t PADDING_W = 3;
static const int32_t PADDING_H = 3;
static const int32_t FONT_4_H = 26;
static const int32_t FONT_6_H = 48;
static const int32_t FONT_6_W = 32;

void drawECO2(int32_t x, int32_t y){
  M5.Lcd.setTextColor(GREENYELLOW, BLACK);
  M5.Lcd.fillRect(x, y, 4, FONT_4_H * 2 + PADDING_H + FONT_6_H, GREENYELLOW);
  M5.Lcd.drawString("eCO2", x + 6, y + PADDING_H, 4);
  int16_t eco2 = sgp.eCO2;
  int32_t eco2_x = x + 6;
  M5.Lcd.setTextColor(WHITE, BLACK);
  if(eco2 < 1000){
    eco2_x += FONT_6_W;
  }
  if(eco2 < 100){
    eco2_x += FONT_6_W;
  }
  if(eco2 < 10){
    eco2_x += FONT_6_W;
  }

  // Warning value
  if(eco2 > 1000){
    M5.Lcd.setTextColor(RED, BLACK);
  }
  M5.Lcd.drawString(String(eco2), eco2_x, y + PADDING_H + FONT_4_H, 7);
  M5.Lcd.setTextColor(GREENYELLOW, BLACK);
  M5.Lcd.drawString("ppm", x + 6 + FONT_6_W * 3 + 5, y + PADDING_H + (FONT_4_H * 3), 4);
}

void drawTVOC(int32_t x, int32_t y){
  M5.Lcd.setTextColor(GREENYELLOW, BLACK);
  M5.Lcd.fillRect(x, y, 4, FONT_4_H * 2 + PADDING_H + FONT_6_H, GREENYELLOW);
  M5.Lcd.drawString("TVIC", x + 6, y + PADDING_H, 4);
  int16_t tvoc = sgp.TVOC;
  int32_t tvoc_x = x + 6 + FONT_6_W;
  M5.Lcd.setTextColor(WHITE, BLACK);
  if(tvoc < 100){
    tvoc_x += FONT_6_W;
  }
  if(tvoc < 10){
    tvoc_x += FONT_6_W;
  }

  // Warning value
  if(tvoc > 89){
    M5.Lcd.setTextColor(RED, BLACK);
  }

  M5.Lcd.drawString(String(tvoc), tvoc_x, y + PADDING_H + FONT_4_H, 7);
  M5.Lcd.setTextColor(GREENYELLOW, BLACK);
  M5.Lcd.drawString("ppb", x + 6 + FONT_6_W * 3 + 5, y + PADDING_H + (FONT_4_H * 3), 4);
}

void drawPower(int32_t x, int32_t y)
{
  M5.Lcd.setTextColor(GREENYELLOW, BLACK);
  M5.Lcd.drawString("POWER", x + 1, y + 3, 4);
  int32_t second_line_y = y + 3 + FONT_4_H + PADDING_H;
  if (M5.Power.isCharging())
  {
    M5.Lcd.setTextColor(YELLOW, BLACK);
    M5.Lcd.drawString("EXTERNAL", x + 1, second_line_y, 4);
  }
  else
  {
    M5.Lcd.setTextColor(RED, BLACK);
    M5.Lcd.drawString("INTERNAL", x + 1, second_line_y, 4);
  }
  M5.Lcd.drawFastVLine(x + 140 + 2, y + 1, (FONT_4_H + PADDING_H) * 2, GREENYELLOW);
  M5.Lcd.setTextColor(WHITE, BLACK);
  int8_t battery = M5.Power.getBatteryLevel();
  int32_t battery_x = x + 180;
  if (battery < 100)
  {
    M5.Lcd.setTextColor(YELLOW, BLACK);
    battery_x += FONT_6_W;
  }
  if (battery < 10)
  {
    M5.Lcd.setTextColor(RED, BLACK);
    battery_x += FONT_6_W;
  }
  M5.Lcd.drawString(String(battery), battery_x, y + 3, 7);
  M5.Lcd.setTextColor(GREENYELLOW, BLACK);
  M5.Lcd.drawString("%", x + 280, second_line_y, 4);
  M5.Lcd.drawRect(x, y, 320, (FONT_4_H + PADDING_H) * 2, GREENYELLOW);
}
