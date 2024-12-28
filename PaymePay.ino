#include <Arduino.h>
#include <WiFi.h>
#include "MenuManager.h"

#define I2C_ADDR 0x27
#define LCD_COLUMNS 20
#define LCD_LINES 4

// Кнопки
const int busyPin = 2;
const int pinTickBalance = 12;
const int pinDataBalance = 13;

const uint8_t wifiIcon[4][8] = {
  {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00100,
  },
  {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b01110,
    0b00000,
    0b00100,
  },
  {
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b00000,
    0b01110,
    0b00000,
    0b00100,
  },
  {
    0b01110,
    0b10001,
    0b00000,
    0b11111,
    0b00000,
    0b01110,
    0b00000,
    0b00100,
  },
};

// Переменные
int amount = 0;
int settingIndex = 0;
unsigned long lastPressTime = 0;
const unsigned long longPressThreshold = 1000;  // 1 сек

static bool tickToggle = true;

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);

MenuContainer container;

int getSignalStrength(long rssi) {
  if (rssi <= -80) {
    return 0;  // Очень слабый сигнал
  } else if (rssi > -80 && rssi <= -65) {
    return 1;  // Слабый сигнал
  } else if (rssi > -65 && rssi <= -50) {
    return 2;  // Хороший сигнал
  } else {
    return 3;  // Отличный сигнал
  }
}


void updateWifiIcon() {
  static long lastTime = 0;
  long currTime = millis();

  if (currTime - lastTime < 1000) return;

  lastTime = currTime;

  char icons[4] = { '\x00', '\x01', '\x02', '\x03' };

  lcd.setCursor(19, 0);
  if (WiFi.status() == WL_CONNECTED) {
    long rssi = WiFi.RSSI();                  // Получаем RSSI (силу сигнала)
    int rssiIndex = getSignalStrength(rssi);  // Градация сигнала
    lcd.print(icons[rssiIndex]);

  } else {
    Serial.println("Not connected :(");
    lcd.print(icons[0]);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  //  pinMode(buttonOk, INPUT_PULLUP);
  //  pinMode(buttonUp, INPUT_PULLDOWN);
  //  pinMode(buttonDown, INPUT_PULLUP);
  pinMode(pinTickBalance, INPUT_PULLUP);
  pinMode(pinDataBalance, OUTPUT);
  pinMode(busyPin, OUTPUT);

  Wire.begin(14, 15);
  lcd.init();
  lcd.backlight();

  for (int i = 0; i < 4; i++) {
    lcd.createChar(i, (uint8_t*)wifiIcon[i]);
  }

  // Serial.println("Wi-Fi connected!");
  // Serial.print("IP Address: ");
  // Serial.println(WiFi.localIP());
  // Serial.print("Signal strength (RSSI): ");
  // Serial.println(WiFi.RSSI());

  Event ev(START);
  container.handleEvent(&ev);
}

void loop() {
  // put your main code here, to run repeatedly:
  // Обработка иконка Wi-Fi
  updateWifiIcon();

  // Обработка кнопок
  //  if (digitalRead(buttonUp) == HIGH) {
  //    delay(20);
  //    while (digitalRead(buttonUp) == HIGH)
  //      ;
  //    Event ev(BUTTON_UP_PRESSED);
  //    container.handleEvent(&ev);
  //  } else if (digitalRead(buttonDown) == LOW) {
  //    delay(20);
  //    while (digitalRead(buttonDown) == LOW)
  //      ;
  //    Event ev(BUTTON_DOWN_PRESSED);
  //    container.handleEvent(&ev);
  //  } else if (digitalRead(buttonOk) == LOW) {
  //    delay(20);
  //    while (digitalRead(buttonOk) == LOW)
  //      ;
  //    Event ev(BUTTON_OK_PRESSED);
  //    container.handleEvent(&ev);
  //  } else

  if (digitalRead(pinTickBalance) == LOW) {
    delay(20);
    if (digitalRead(pinTickBalance) == LOW && tickToggle) {
      tickToggle = false;
      Event ev(TICK_DOWN);
      container.handleEvent(&ev);
    }
  }

  if (digitalRead(pinTickBalance) == HIGH) {
    delay(20);
    if (digitalRead(pinTickBalance) == HIGH && !tickToggle) {
      tickToggle = true;
      Event ev(TICK_UP);
      container.handleEvent(&ev);
    }
  }

  if (Serial.available() > 0) {
    Event ev(UART_ON_RECEIVED);
    container.handleEvent(&ev);
  }
}