#include <M5Stack.h>
#include "Adafruit_SGP30.h"

#define CONSOLE Serial

static const long CONSOL_BAND = 115200;

static const int32_t PADDING_W = 3;
static const int32_t PADDING_H = 3;
static const int32_t FONT_2_H = 16;
static const int32_t FONT_4_H = 26;
static const int32_t FONT_6_W = 32;
static const int32_t FONT_6_H = 48;
static const int32_t HALF_SIZE_MONITOR_W = 160;
static const int32_t HALF_SIZE_MONITOR_H = 100;
static const int32_t THIRD_SIZE_MONITOR_W = 106;
static const int32_t THIRD_SIZE_MONITOR_H = 80;

Adafruit_SGP30 sgp;

// Doublebuffer
TFT_eSprite canvas = TFT_eSprite(&M5.Lcd);

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

  CONSOLE.println("Setup finish");
  canvas.setColorDepth(8);
  canvas.createSprite(M5.Lcd.width(), M5.Lcd.height());
}

void loop()
{
  canvas.fillScreen(BLACK);
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
  drawThirdsizeMoniter("HOGE", "ppb", String(25), WHITE, 0, 160);
  drawThirdsizeMoniter("SHITSUDO", "%", String(15), WHITE, THIRD_SIZE_MONITOR_W, 160);
  drawThirdsizeMoniter("fukai", "hs", String(25), WHITE, THIRD_SIZE_MONITOR_W * 2, 160);
  canvas.pushSprite(0, 0);
  delay(1000);
}

void drawThirdsizeMoniter(String title, String unit, String value, uint16_t fcolor, int32_t x, int32_t y)
{
  canvas.setTextFont(2);
  int16_t title_width = canvas.textWidth(title);
  int16_t unit_width = canvas.textWidth(unit);
  canvas.setTextFont(7);
  int16_t value_width = canvas.textWidth(value);

  canvas.fillRect(x, y, THIRD_SIZE_MONITOR_W, FONT_2_H, GREENYELLOW);
  canvas.setTextColor(BLACK, GREENYELLOW);
  canvas.drawString(title, x + ((THIRD_SIZE_MONITOR_W - title_width) / 2), y, 2);
  canvas.setTextColor(fcolor, BLACK);
  canvas.drawString(value, x + (THIRD_SIZE_MONITOR_W - value_width), y + FONT_2_H, 7);
  canvas.setTextColor(GREENYELLOW, BLACK);
  canvas.drawString(unit, x + (THIRD_SIZE_MONITOR_W - unit_width), y + FONT_2_H + FONT_6_H, 2);
  canvas.drawRect(x, y, THIRD_SIZE_MONITOR_W, THIRD_SIZE_MONITOR_H, GREENYELLOW);
}

void drawECO2(int32_t x, int32_t y)
{
  int16_t eco2 = sgp.eCO2;
  uint16_t fcolor = WHITE;
  // Warning value
  if (eco2 > 1000)
  {
    fcolor = RED;
  }
  drawHalfsizeMonitor("eCO2", "ppm", eco2, fcolor, x, y);
}

void drawTVOC(int32_t x, int32_t y)
{
  int16_t tvoc = sgp.TVOC;
  uint16_t fcolor = WHITE;
  // Warning value
  if (tvoc > 89)
  {
    fcolor = RED;
  }
  drawHalfsizeMonitor("TVOC", "ppb", tvoc, fcolor, x, y);
}

// Width 160, Hight 100
void drawHalfsizeMonitor(String title, String unit, int16_t value, uint16_t fcolor, int32_t x, int32_t y)
{
  canvas.setTextColor(GREENYELLOW, BLACK);
  canvas.fillRect(x, y, 4, HALF_SIZE_MONITOR_H, GREENYELLOW);
  canvas.drawString(title, x + 6, y + PADDING_H, 4);
  int32_t value_x = x + (HALF_SIZE_MONITOR_W - FONT_6_W * 4);
  if (value < 1000)
  {
    value_x += FONT_6_W;
  }
  if (value < 100)
  {
    value_x += FONT_6_W;
  }
  if (value < 10)
  {
    value_x += FONT_6_W;
  }
  canvas.setTextColor(fcolor, BLACK);

  canvas.drawString(String(value), value_x, y + FONT_4_H, 7);
  canvas.setTextColor(GREENYELLOW, BLACK);
  canvas.setTextFont(4);
  int16_t unit_width = canvas.textWidth(unit);
  canvas.drawString(unit, x + HALF_SIZE_MONITOR_W - unit_width - PADDING_W, y + FONT_4_H + FONT_6_H, 4);
}

void drawPower(int32_t x, int32_t y)
{
  canvas.setTextColor(GREENYELLOW, BLACK);
  canvas.drawString("POWER", x + 1, y + 3, 4);
  int32_t second_line_y = y + 3 + FONT_4_H + PADDING_H;
  if (M5.Power.isCharging())
  {
    canvas.setTextColor(YELLOW, BLACK);
    canvas.drawString("EXTERNAL", x + 1, second_line_y, 4);
  }
  else
  {
    canvas.setTextColor(RED, BLACK);
    canvas.drawString("INTERNAL", x + 1, second_line_y, 4);
  }
  canvas.drawFastVLine(x + 140 + 2, y + 1, (FONT_4_H + PADDING_H) * 2, GREENYELLOW);
  canvas.setTextColor(WHITE, BLACK);
  int8_t battery = M5.Power.getBatteryLevel();
  int32_t battery_x = x + 180;
  if (battery < 100)
  {
    canvas.setTextColor(YELLOW, BLACK);
    battery_x += FONT_6_W;
  }
  if (battery < 10)
  {
    canvas.setTextColor(RED, BLACK);
    battery_x += FONT_6_W;
  }
  canvas.drawString(String(battery), battery_x, y + 3, 7);
  canvas.setTextColor(GREENYELLOW, BLACK);
  canvas.drawString("%", x + 280, second_line_y, 4);
  canvas.drawRect(x, y, 320, (FONT_4_H + PADDING_H) * 2, GREENYELLOW);
}
