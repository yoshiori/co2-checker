#include <M5Stack.h>
#include "Adafruit_SGP30.h"
#include "M5_ENV.h"
#include <WiFi.h>
#include <IIJMachinistClient.h>

#include "config.h"

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

// I don't know why. It's reverse.
static const int16_t DHISPLAY_H = M5.Lcd.width();
static const int16_t DHISPLAY_W = M5.Lcd.height();

Adafruit_SGP30 sgp;
SHT3X sht30;
IIJMachinistClient *machinist;
QMP6988 qmp6988;

static const String MACHINIST_AGENT_NAME = "M5Stack";
static const String MACHINIST_NAMESPACE = "Environment Sensor";

// Doublebuffer
TFT_eSprite canvas = TFT_eSprite(&M5.Lcd);

void setup()
{
  M5.begin();
  CONSOLE.begin(CONSOL_BAND);
  CONSOLE.println("Setup start");
  M5.Lcd.println("Setup start");
  CONSOLE.printf("DISP H %d \tDISP W %d \n", DHISPLAY_H, DHISPLAY_W);
  M5.Lcd.print("Connecting SPG30");
  if (!sgp.begin())
  {
    CONSOLE.println("Sensor not found :(");
    while (1)
      ;
  }
  sgp.softReset();
  sgp.IAQinit();
  M5.Lcd.println(" : Finish");

  M5.Power.begin();
  qmp6988.init();

  M5.Lcd.printf("Connecting WiFi to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    CONSOLE.print(".");
    M5.Lcd.print(".");
  }
  CONSOLE.println("");
  M5.Lcd.println(" : Finish");
  machinist = new IIJMachinistClient(MACHINIST_APIKEY);
  canvas.setColorDepth(8);
  canvas.createSprite(M5.Lcd.width(), M5.Lcd.height());
  CONSOLE.println("Setup finish");
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

  uint16_t soil_moisture = analogRead(36);
  float pressure = qmp6988.calcPressure() / 100;
  canvas.fillScreen(BLACK);
  CONSOLE.printf("TVOC:%dppb\teCO2:%dppm\ttemp:%f\thum:%f\twater:%d\tpressure:%f\n", tvoc, eco2, temperature, humidity, soil_moisture, pressure);
  drawSoilMoisture(soil_moisture, 0, 0);
  drawTemperature(temperature, 0, 60);
  drawECO2(eco2, 160, 60);
  drawHumidity(humidity, 0, 160);
  drawTHI(thi, THIRD_SIZE_MONITOR_W, 160);
  drawTVOC(tvoc, THIRD_SIZE_MONITOR_W * 2, 160);
  canvas.pushSprite(0, 0);
  sendSensorData(temperature, humidity, pressure, tvoc, eco2, soil_moisture);
  delay(1000);
}

void sendSensorData(float temperature, float humidity, float pressure, uint16_t tvoc, uint16_t eco2, uint16_t soil_moisture)
{
  static unsigned long nextUpdate = 0;
  if (nextUpdate < millis())
  {
    machinist->post(MACHINIST_AGENT_NAME, MACHINIST_NAMESPACE, "Temperature", temperature);
    machinist->post(MACHINIST_AGENT_NAME, MACHINIST_NAMESPACE, "Humidity", humidity);
    machinist->post(MACHINIST_AGENT_NAME, MACHINIST_NAMESPACE, "Pressure", pressure);
    machinist->post(MACHINIST_AGENT_NAME, MACHINIST_NAMESPACE, "TVOC", tvoc);
    machinist->post(MACHINIST_AGENT_NAME, MACHINIST_NAMESPACE, "eCO2", eco2);
    machinist->post(MACHINIST_AGENT_NAME, MACHINIST_NAMESPACE, "Soil Moisture", soil_moisture);
    nextUpdate = millis() + 60000;
  }
}

uint32_t getAbsoluteHumidity(float temperature, float humidity)
{
  float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature));
  return static_cast<uint32_t>(1000.0f * absoluteHumidity);
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
      M5.Lcd.fillRect(0, 0, DHISPLAY_W, DHISPLAY_H, BLACK);
      int32_t x = (DHISPLAY_W - FONT_6_W * 2) / 2;
      if (i < 10)
      {
        x += FONT_6_W;
      }
      M5.Lcd.drawNumber(i, x, (DHISPLAY_H - FONT_6_H) / 2, 7);
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

void drawSoilMoisture(uint16_t soil_moisture, int32_t x, int32_t y)
{
  canvas.setTextColor(GREENYELLOW, BLACK);
  canvas.drawString("SOIL ", x + 1, y + PADDING_H, 4);
  canvas.drawString("MOISTURE", x + 1, y + FONT_4_H + PADDING_H * 2, 4);
  canvas.drawFastVLine(x + 140 + 2, y + 1, (FONT_4_H + PADDING_H) * 2, GREENYELLOW);

  uint16_t fcolor = WHITE;
  // Warning value
  if (soil_moisture > 1000)
  {
    fcolor = YELLOW;
  }
  if (soil_moisture > 2000)
  {
    fcolor = ORANGE;
  }
  if (soil_moisture > 3000)
  {
    fcolor = RED;
  }
  canvas.setTextColor(fcolor, BLACK);

  String soil_moisture_text = String(soil_moisture);
  canvas.setTextFont(7);
  int16_t soil_moisture_width = canvas.textWidth(soil_moisture_text);

  canvas.drawString(soil_moisture_text, DHISPLAY_W - soil_moisture_width, y + 3, 7);
  canvas.drawRect(x, y, DHISPLAY_W, (FONT_4_H + PADDING_H) * 2, GREENYELLOW);
}
