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

class autoBrightness {
private:
  bool state = false;
  uint8_t brightness = 255;
  int pin;
public:
  autoBrightness(int p) {
    pinMode(p, OUTPUT);
    pin = p;
  }
  void setState(bool b) {
    state = b;
  }
  void run() {
    if (state == true)
      start();
    else
      stop();
  }
  void start() {
    int temp = (analogRead(A1) / 1.5);
    if (abs(temp - brightness) > 25) {
      if (temp < 20)
        temp = 20;
      analogWrite(pin, temp);
      brightness = temp;
    }
    Serial.println(temp);
  }
  void stop() {
    brightness = 255;
    analogWrite(pin, brightness);
  }
};

autoBrightness AB = autoBrightness(6);


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
  AB.setState(false);
  
  for (int i = 2; i <= led; i++)
    pinMode(i, OUTPUT);
  turnOff();

  ADS_setConfig(0xD2, 0xE5);
}

void loop() {
  AB.run();
  resolveData();
  printText("FUEL", 13, 10, 1, 20, 150, 200);
  printText("ADULTERATION", 43, 10, 1, 20, 150, 200);
  printText("DETECTION", 13, 20, 1, 20, 150, 200);
  printText("SYSTEM", 75, 20, 1, 20, 150, 200);
}

void resolveData() {
  int16_t pressure = optimalPressure();
  int density = pressure * 11.35 / 9.81 * 0.1;
  if (density < 0) {
    printText("Tank is", 30, 60, 2, 20, 50, 255);
    printText("empty!", 35, 80, 2, 20, 50, 255);

  } else {
    printText("Pressure:", 20, 40, 1, 255, 150, 20);
    printText(String(pressure) + " Pa", 20, 50, 1);
    printText(String(density) + " Kg/m3", 20, 80, 1);
    printText("Density:", 20, 70, 1, 255, 150, 20);

    turnOn_2();
    printText("Color:", 20, 100, 1, 255, 150, 20);
    printText(detectClr(red(), green(), blue()), 20, 110, 1);
    turnOff();
  }
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
    Serial.println(val);
    return val;
  }
}

int16_t optimalPressure() {
  float voltage = .0;
  for (int i = 0; i < 100; i++) {
    voltage += (ADS_A1() * 4.096) / 32768.0;
    delay(20);
  }
  clear();
  return ((((voltage / 100.0) - .5) * 1600000) / (4.5 - .5));
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
RGB TO HEX CONVERSION
**************************/

String detectClr(uint16_t r, uint16_t g, uint16_t b) {
  int t = r + g + b;
  String hexCode;
  r = ((float)r / t) * 100.0;
  g = ((float)g / t) * 100.0;
  b = ((float)b / t) * 100.0;

  r = 100 - r;
  g = 100 - g;
  b = 100 - b;
  t = r + g + b;

  r = 2.5 * (r * 100) / t;
  g = 2.5 * (g * 100) / t;
  b = 2.5 * (b * 100) / t;

  hexCode = "#" + String(r / 16, HEX);
  hexCode += String(r % 16, HEX);
  hexCode += String(g / 16, HEX);
  hexCode += String(g % 16, HEX);
  hexCode += String(b / 16, HEX);
  hexCode += String(b % 16, HEX);

  return hexCode;
}


/*************************
COLOR INTENSITY MEASUREMENT
**************************/

uint16_t red() {
  digitalWrite(s2, LOW);
  digitalWrite(s3, LOW);
  return (uint16_t)(pulseIn(OUT, HIGH) * 2.5);
}

uint16_t green() {
  digitalWrite(s2, HIGH);
  digitalWrite(s3, HIGH);
  return (uint16_t)(pulseIn(OUT, HIGH) * 2.5);
}

uint16_t blue() {
  digitalWrite(s2, LOW);
  digitalWrite(s3, HIGH);
  return (uint16_t)(pulseIn(OUT, HIGH) * 2.5);
}

uint16_t exposure() {
  digitalWrite(s2, HIGH);
  digitalWrite(s3, LOW);
  return (uint16_t)(pulseIn(OUT, LOW) * 2.5);
}

/*************************
OUTPUT FREQUENCY SCALING
**************************/

void turnOff() {
  digitalWrite(led, LOW);
  digitalWrite(s0, LOW);
  digitalWrite(s1, LOW);
}

void turnOn_2() {
  digitalWrite(led, HIGH);
  digitalWrite(s0, HIGH);
  digitalWrite(s1, LOW);
}

void clear() {
  scr.background(0, 0, 0);
}
