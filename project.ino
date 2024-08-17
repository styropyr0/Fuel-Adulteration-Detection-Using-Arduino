#include <Wire.h>
#include <TFT.h>

#define ADS1115 0x48
#define CONFIG_REG 0x01
#define CONV_REG 0x00

#define cs 10
#define dc 9
#define rst 8

const int led = 7, s0 = 2, s1 = 3, s2 = 4, s3 = 5, OUT = A0;
TFT scr = TFT(cs, dc, rst);

enum calibColor : int {
  MIN_R = 180,
  MIN_G = 186,
  MIN_B = 145,
  MAX_R = 3471,
  MAX_G = 3292,
  MAX_B = 2727,
  PURE_R = 255,
  PURE_G = 255,
  PURE_B = 30,
};

enum calibPressure : uint16_t {
  MIN_P = 100,
  MAX_P = 65535,
  PURE_P = 4000,
  MAX_VAR = 250,
};

void printText(String text, int x, int y, int textSize = 2, int r = 255,
               int g = 255, int b = 255) {
  char buffer[15];
  text.toCharArray(buffer, 15);
  scr.stroke(r, g, b);
  scr.setTextSize(textSize);
  scr.text(buffer, x, y);
  scr.noStroke();
}

void setup() {
  Wire.begin();
  scr.begin();
  scr.setRotation(2);
  clear();
  printText("Starting", 18, 55);
  printText("up...", 35, 75);
  Serial.begin(9600);

  for (int i = 2; i <= led; i++)
    pinMode(i, OUTPUT);
  turnOff();

  ADS_setConfig(0xD2, 0xE5);
  clear();
  printText("Measuring...", 27, 65, 1, 255, 255, 255);
}

void loop() {
  resolveData();
  printText("FUEL", 13, 7, 1, 20, 150, 200);
  printText("ADULTERATION", 43, 7, 1, 20, 150, 200);
  printText("DETECTION", 13, 17, 1, 20, 150, 200);
  printText("SYSTEM", 75, 17, 1, 20, 150, 200);
}

void resolveData() {
  int16_t pressure = optimalPressure();
  int r = 0;
  int g = 0;
  int b = 0;
  for (int i = 0; i < 50; i++) {
    r += red();
    turnOff();
    delay(20);
    g += green();
    turnOff();
    delay(20);
    b += blue();
    turnOff();
    delay(20);
  }
  r /= 50;
  g /= 50;
  b /= 50;
  float purity = calculatePurity(pressure, r, g, b);
  int density = pressure / 9.81 * 0.1;
  clear();
  if (density < -25005) {
    printText("Tank is", 30, 60, 2, 20, 50, 255);
    printText("empty!", 35, 80, 2, 20, 50, 255);
  } else {
    printText("Pressure:", 13, 30, 1, 255, 150, 20);
    printText(String(pressure) + " Pa", 13, 40, 1);
    printText(String(density) + " Kg/m3", 13, 65, 1);
    printText("Density:", 13, 55, 1, 255, 150, 20);
    printText("Color:", 13, 80, 1, 255, 150, 20);
    uint8_t PERC_R, PERC_G, PERC_B;
    if (purity >= 90) {
      PERC_R = 20;
      PERC_G = 255;
      PERC_B = 20;
    } else if (purity >= 70) {
      PERC_R = 255;
      PERC_G = 245;
      PERC_B = 25;
    } else if (purity >= 50) {
      PERC_R = 255;
      PERC_G = 110;
      PERC_B = 25;
    } else {
      PERC_R = 255;
      PERC_G = 24;
      PERC_B = 25;
    }
    printText((String)purity + "%", 13, 120, 2, PERC_B, PERC_G, PERC_R);
    printText("Purity:", 13, 108, 1, 255, 150, 20);
    printText("as compared to GOI", 13, 137, 1, 20, 250, 155);
    printText("GOI", 103, 137, 1, 20, 250, 155);
    printText("standards.", 13, 147, 1, 20, 250, 155);
    printText(detectClr(red(), green(), blue()), 13, 93, 1);
    turnOff();
  }
}

float optimalPressure() {
  float pVal = .0;
  for (int i = 0; i < 100; i++) {
    pVal += ADS_A1();
    delay(40);
  }
  return map(pVal / 100, MIN_P, MAX_P, 0, 65535);
}

float calculatePurity(float pressure, int r, int g, int b) {
  float P_PURITY = abs(100 - (pressure / PURE_P) * 100);
  float R_PURITY = abs(100 - (r / PURE_R) * 100);
  float G_PURITY = abs(100 - (g / PURE_G) * 100);
  float B_PURITY = abs(100 - (b / PURE_B) * 100);
  float total = 100 * (100 * ((R_PURITY + G_PURITY + B_PURITY) / 300) + P_PURITY) / 200;
  if (total > 100)
    total = 100;
  return 100 - total;
}

/*************************
READ FROM ADS1115 A0 PORT
**************************/

uint16_t ADS_A1() {
  Wire.beginTransmission(ADS1115);
  Wire.write(CONV_REG);
  Wire.endTransmission();
  Wire.requestFrom(ADS1115, 2);

  if (Wire.available() == 2) {
    uint16_t val = Wire.read() << 8;
    val |= Wire.read();
    return val;
  }
}

/*************************
CHANGE ADS1115 CONFIG
**************************/

void ADS_setConfig(uint8_t DATA_MSB, uint8_t DATA_LSB) {
  Wire.beginTransmission(ADS1115);
  Wire.write(CONFIG_REG);
  Wire.write(DATA_MSB);
  Wire.write(DATA_LSB);
  Wire.endTransmission();
}

/*************************
COLOR DETECTION
**************************/

String detectClr(float r, float g, float b) {
  char hexCode[13];
  if (b <= 0 || b > MAX_R)
    b = 0;
  if (g <= 0 || g > MAX_G)
    g = 0;
  if (r <= 0 || r > MAX_R)
    r = 0;
  scr.fill(b, g, r);
  scr.rect(60, 78, 10, 10);
  sprintf(hexCode, "(%d,%d,%d)", (int)r, (int)g, (int)b);
  return hexCode;
}

/*************************
COLOR INTENSITY MEASUREMENT
**************************/

int red() {
  turnOn_2();
  digitalWrite(s2, LOW);
  digitalWrite(s3, LOW);
  return map(pulseIn(OUT, LOW), MIN_R, MAX_R, 255, 0);
}

int green() {
  turnOn_2();
  digitalWrite(s2, HIGH);
  digitalWrite(s3, HIGH);
  return map(pulseIn(OUT, LOW), MIN_G, MAX_G, 255, 0);
}

int blue() {
  turnOn_2();
  digitalWrite(s2, LOW);
  digitalWrite(s3, HIGH);
  return map(pulseIn(OUT, LOW), MIN_B, MAX_B, 255, 0);
}

uint16_t exposure() {
  turnOn_2();
  digitalWrite(s2, HIGH);
  digitalWrite(s3, LOW);
  return (uint16_t)map(pulseIn(OUT, LOW), 10, 1500, 100, 0);
}

void turnOff() {
  digitalWrite(led, LOW);
  digitalWrite(s0, LOW);
  digitalWrite(s1, LOW);
}

/*************************
OUTPUT FREQUENCY SCALING
**************************/

void turnOn_2() {
  digitalWrite(led, HIGH);
  digitalWrite(s0, LOW);
  digitalWrite(s1, HIGH);
}

void clear() {
  scr.background(0, 0, 0);
}
