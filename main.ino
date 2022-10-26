#include <M5Stack.h>
#include "Adafruit_SGP30.h"
#include "SHT3X.h"
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
SHT3X sht30;
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
  warmUp();
  if (sht30.get() != 0)
  {
    CONSOLE.println("SHT30 Measurement failed");
    return;
  }
  float temperature = sht30.cTemp;
  float humidity = sht30.humidity;
  float thi = getTHI(temperature, humidity);

  sgp.setHumidity(getAbsoluteHumidity(temperature, humidity));

  if (!sgp.IAQmeasure())
  {
    CONSOLE.println("SPG Measurement failed");
    return;
  }
  uint16_t tvoc = sgp.TVOC;
  uint16_t eco2 = sgp.eCO2;

  canvas.fillScreen(BLACK);
  CONSOLE.printf("TVOC %d ppb\teCO2 %d ppm\n", tvoc, eco2);
  CONSOLE.printf("temp %f \t hum %f\n", temperature, humidity);
  drawPower(0, 0);
  drawTemperature(temperature, 0, 60);
  drawECO2(eco2, 160, 60);
  drawTVOC(tvoc, 0, 160);
  drawHumidity(humidity, THIRD_SIZE_MONITOR_W, 160);
  drawTHI(thi, THIRD_SIZE_MONITOR_W * 2, 160);
  canvas.pushSprite(0, 0);
  delay(1000);
}

uint32_t getAbsoluteHumidity(float temperature, float humidity)
{
  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature));
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity);
  return absoluteHumidityScaled;
}

float getTHI(float temperature, float humidity)
{
  // https://ja.wikipedia.org/wiki/%E4%B8%8D%E5%BF%AB%E6%8C%87%E6%95%B0
  return 0.81 * temperature + 0.01f * humidity * (0.99f * temperature - 14.3f) + 46.3f;
}

void warmUp()
{
  static int i = 15;
  long last_millis = 0;
  while (i > 0)
  {
    if (millis() - last_millis > 1000)
    {
      last_millis = millis();
      i--;
      M5.Lcd.fillRect(20, 120, 60, 30, BLACK);
      M5.Lcd.drawNumber(i, 20, 120, 2);
    }
  }
}

void drawTHI(float thi, int32_t x, int32_t y)
{
  uint16_t fcolor = WHITE;
  if (thi < 55.0f)
  {
    fcolor = RED;
  }
  if (55.0f < thi && thi < 60.0f)
  {
    fcolor = YELLOW;
  }
  if (70.0f < thi && thi < 75.0f)
  {
    fcolor = YELLOW;
  }
  if (75.0f < thi && thi < 80.0f)
  {
    fcolor = ORANGE;
  }
  if (80.0f < thi)
  {
    fcolor = RED;
  }

  drawThirdsizeMoniter("THI", "", String(thi, 0), fcolor, x, y);
}

void drawHumidity(float humidity, int32_t x, int32_t y)
{
  drawThirdsizeMoniter("HUMIDITY", "%", String(humidity, 0), WHITE, x, y);
}

void drawTemperature(float temperature, int32_t x, int32_t y)
{
  drawHalfsizeMonitor("Temperature", "'c", String(temperature, 1), WHITE, x, y);
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
  canvas.drawString(value, x + (THIRD_SIZE_MONITOR_W - value_width), y + FONT_2_H + 2, 7);
  canvas.setTextColor(GREENYELLOW, BLACK);
  canvas.drawString(unit, x + (THIRD_SIZE_MONITOR_W - unit_width), y + FONT_2_H + FONT_6_H, 2);
  canvas.drawRect(x, y, THIRD_SIZE_MONITOR_W, THIRD_SIZE_MONITOR_H, GREENYELLOW);
}

void drawECO2(uint16_t eco2, int32_t x, int32_t y)
{
  uint16_t fcolor = WHITE;
  // Warning value
  if (eco2 > 1000)
  {
    fcolor = RED;
  }
  drawHalfsizeMonitor("eCO2", "ppm", String(eco2), fcolor, x, y);
}

void drawTVOC(uint16_t tvoc, int32_t x, int32_t y)
{
  uint16_t fcolor = WHITE;
  // Warning value
  if (tvoc > 89)
  {
    fcolor = RED;
  }
  drawThirdsizeMoniter("TVOC", "ppb", String(tvoc), fcolor, x, y);
}

// Width 160, Hight 100
void drawHalfsizeMonitor(String title, String unit, String value, uint16_t fcolor, int32_t x, int32_t y)
{
  canvas.setTextColor(GREENYELLOW, BLACK);
  canvas.fillRect(x, y + 4, 4, HALF_SIZE_MONITOR_H - 8, GREENYELLOW);
  canvas.drawString(title, x + 6, y + PADDING_H, 4);

  canvas.setTextFont(7);
  int16_t value_width = canvas.textWidth(value);

  canvas.setTextColor(fcolor, BLACK);
  canvas.drawString(String(value), x + (HALF_SIZE_MONITOR_W - value_width), y + FONT_4_H, 7);
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
