#define BLYNK_PRINT Serial

// Библиотеки
#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>
#include <Wire.h>
#include <Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_BMP085_U.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// LCD дисплей
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Датчик атмосферного давления
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

// Датчик касания (кнопка)
#define BUTTON_PIN 2

// Сервомотор
#define SERVO_PIN 3
Servo servo_motor;

// Выход пьезодинамика
#define BUZZER_PIN 4

// Выход реле управления помпой
#define RELAY_PIN 5

// Датчик движения
#define HCSR501_PIN 6

// Датчик температуры почвы
#define DS18B20_PIN 7
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds_sensor(&oneWire);

// Выход на транзистор управления светом
#define MOSFET_PIN 8

// Датчик температуры/влажности воздуха
#define DHT_PIN 9
DHT dht(DHT_PIN, DHT11);

// Датчик расстояния ультразвуковой
#define US1_echoPin 22
#define US1_trigPin 23
#define minimumRange 0
#define maximumRange 400

// Кнопочная клавиатура
#define KEYBOARD_PIN A0

// Датчик уровня воды
#define WATER_PIN A1

// Датчик влажности почвы
#define SOIL_PIN A2

// Датчик пламени
#define FIRE_PIN A3

// Датчик освещенности
#define LIGHT_PIN A4

// Датчик дыма/газа
#define GAS_PIN A5

// Датчик звука
#define SOUND_PIN A8

// Последняя нажатая кнопка клавиатуры
int lastkey = 0;

// Период для таймера обновления данных
#define UPDATE_TIMER   5000
#define KEYBOARD_TIMER 100

// Таймеры
BlynkTimer timer_update;
BlynkTimer timer_keyboard;

// Параметры IoT сервера
char auth[] = "07d3d174744145909860f09624f921e4";
IPAddress blynk_ip(139, 59, 206, 133);

void setup()
{
  // Инициализация последовательного порта
  Serial.begin(115200);

  // Подключение к серверу Blynk
  Blynk.begin(auth, blynk_ip, 8442);

  // Инициализация I2C
  Wire.begin();

  // Инициализация дисплея
  lcd.init();       // Инициализация
  lcd.backlight();  // Включение подсветки
  lcd.clear();      // Очистка экрана
  lcd.setCursor(0, 0);
  lcd_printstr("Hello world!");

  // Инициализация датчика BMP085/180
  if (!bmp.begin())
    Serial.println("Could not find a valid BMP085/180 sensor!");

  // Инициализация датчика DHT11
  dht.begin();

  // Инициализация датчика DS18B20
  ds_sensor.begin();

  // Инициализация выхода реле
  pinMode(RELAY_PIN, OUTPUT);

  // Инициализация выводов для работы с УЗ датчиком расстояния
  pinMode(US1_trigPin, OUTPUT);
  pinMode(US1_echoPin, INPUT);

  // Инициализация порта для управления сервомотором
  servo_motor.attach(SERVO_PIN);
  servo_motor.write(90);
  delay(1024);

  // Инициализация таймеров
  timer_update.setInterval(UPDATE_TIMER, readSendData);
  timer_keyboard.setInterval(KEYBOARD_TIMER, readKeyBoard);
}

void loop()
{
  Blynk.run();
  timer_update.run();
  timer_keyboard.run();
}

// Считывание датчиков и отправка данных на сервер Blynk
void readSendData()
{
  float air_temp = dht.readTemperature();         // Считывание температуры воздуха
  float air_hum = dht.readHumidity();             // Считывание влажности воздуха
  Serial.print("Air temperature = ");
  Serial.println(air_temp);
  Serial.print("Air humidity = ");
  Serial.println(air_hum);
  Blynk.virtualWrite(V0, air_temp); delay(25);    // Отправка данных на сервер Blynk
  Blynk.virtualWrite(V1, air_hum); delay(25);     // Отправка данных на сервер Blynk

  float light = analogRead(LIGHT_PIN) / 1023.0 * 10000.0; // Считывание датчика света
  Serial.print("Light = ");
  Serial.println(light);
  Blynk.virtualWrite(V2, light); delay(25);       // Отправка данных на сервер Blynk

  float dist = readUS_distance();                 // Считывание расстояния с УЗ датчика
  Serial.print("Distance = ");
  Serial.println(dist);
  Blynk.virtualWrite(V3, dist); delay(25);        // Отправка данных на сервер Blynk

  // Считывание аналоговых датчиков
  float fire  = analogRead(FIRE_PIN) / 1023.0 * 100.0;
  float gas   = analogRead(GAS_PIN) / 1023.0 * 100.0;
  float water = analogRead(WATER_PIN) / 1023.0 * 100.0;
  float soil  = analogRead(SOIL_PIN) / 1023.0 * 100.0;
  Serial.print("Fire detect = ");
  Serial.println(fire);
  Serial.print("Gas detect = ");
  Serial.println(gas);
  Serial.print("Water level = ");
  Serial.println(water);
  Serial.print("Soil moisture = ");
  Serial.println(soil);
  Blynk.virtualWrite(V4, fire); delay(25);        // Отправка данных на сервер Blynk
  Blynk.virtualWrite(V5, gas); delay(25);         // Отправка данных на сервер Blynk
  Blynk.virtualWrite(V6, water); delay(25);       // Отправка данных на сервер Blynk
  Blynk.virtualWrite(V7, soil); delay(25);        // Отправка данных на сервер Blynk

  float sound = readSensorSOUND();                // Считывание интенсивности звука
  Serial.print("Sound = ");
  Serial.println(sound);
  Blynk.virtualWrite(V8, sound); delay(25);       // Отправка данных на сервер Blynk

  float motion = digitalRead(HCSR501_PIN);        // Считывание датчика движения
  Serial.print("Motion = ");
  Serial.println(motion);
  Blynk.virtualWrite(V9, motion); delay(25);      // Отправка данных на сервер Blynk

  // Считывание датчика давления
  float t = 0;
  float p = 0;
  sensors_event_t p_event;
  bmp.getEvent(&p_event);
  if (p_event.pressure)
  {
    p = p_event.pressure * 7.5006 / 10;
    bmp.getTemperature(&t);
  }
  Serial.print("Pressure = ");
  Serial.println(p);
  Blynk.virtualWrite(V10, p); delay(25);          // Отправка данных на сервер Blynk

  float touch = digitalRead(BUTTON_PIN);          // Считывание датчика касания
  Serial.print("Touch = ");
  Serial.println(touch);
  Blynk.virtualWrite(V11, touch); delay(25);      // Отправка данных на сервер Blynk

  // Считывание последней нажатой кнопки клавиатуры
  Serial.print("Last key = ");
  Serial.println(lastkey);
  Blynk.virtualWrite(V12, lastkey); delay(25);    // Отправка данных на сервер Blynk

  // Вывод температуры и влажности воздуха на LCD
  lcd.setCursor(0, 0); lcd_printstr("T = " + String(air_temp, 1) + " *C    ");
  lcd.setCursor(0, 1); lcd_printstr("H = " + String(air_hum, 1) + " %    ");
}

// Считывание нажатой кнопки клавиатуры
void readKeyBoard()
{
  int kb = read_keyboard();
  if ((kb != lastkey) && (kb != 0))
  {
    Serial.print("Key = ");
    Serial.println(kb);
    lastkey = kb;
  }
}

// Управление помпой с Blynk
BLYNK_WRITE(V100)
{
  // Получение управляющего сигнала с сервера
  int relay_ctl = param.asInt();
  Serial.print("Relay power: ");
  Serial.println(relay_ctl);

  // Управление реле (помпой)
  digitalWrite(RELAY_PIN, relay_ctl);
}

// Управление пьезодинамиком с Blynk
BLYNK_WRITE(V101)
{
  // Получение управляющего сигнала с сервера
  int buzzer_ctl = param.asInt();
  Serial.print("Buzzer power: ");
  Serial.println(buzzer_ctl);

  // Управление реле (помпой)
  if (buzzer_ctl)
    tone(BUZZER_PIN, 440, 0);
  else
    noTone(BUZZER_PIN);
}

// Управление сервомотором с Blynk
BLYNK_WRITE(V102)
{
  // Получение управляющего сигнала с сервера
  int servo_ctl = constrain(param.asInt(), 90, 135);
  Serial.print("Servo angle: ");
  Serial.println(servo_ctl);

  // Управление сервомотором
  servo_motor.write(servo_ctl);
}

// Управление лампой с Blynk
BLYNK_WRITE(V103)
{
  // Получение управляющего сигнала с сервера
  int light_ctl = param.asInt();
  Serial.print("Lamp power: ");
  Serial.println(light_ctl);

  // Управление реле (помпой)
  analogWrite(MOSFET_PIN, light_ctl);
}

// Считывание УЗ датчика расстояния
float readUS_distance()
{
  float duration = 0;
  float distance = 0;
  digitalWrite(US1_trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(US1_trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(US1_trigPin, LOW);
  duration = pulseIn(US1_echoPin, HIGH, 50000);
  distance = duration / 58.2;
  if (distance >= maximumRange || distance <= minimumRange) {
    distance = -1;
  }
  return distance;
}

// Считывание датчика звука
float readSensorSOUND()
{
  int max_a = 0;
  for (int i = 0; i < 128; i ++)
  {
    int a = analogRead(SOUND_PIN);
    if (a > max_a)
    {
      max_a = a;
    }
  }
  float db =  20.0  * log10 (max_a  + 1.0);
  return db;
}

// Функция вывода текстовой строки на дисплей
void lcd_printstr(String str1) {
  for (int i = 0; i < str1.length(); i++) {
    lcd.print(str1.charAt(i));
  }
}

// Определение номера нажатой кнопки на клавиатуре
int read_keyboard() {
  int sensor_data = analogRead(KEYBOARD_PIN);
  if (sensor_data < 20) {
    return 1;
  } else if ((sensor_data > 130) && (sensor_data < 150)) {
    return 2;
  } else if ((sensor_data > 315) && (sensor_data < 335)) {
    return 3;
  } else if ((sensor_data > 490) && (sensor_data < 510)) {
    return 4;
  } else if ((sensor_data > 730) && (sensor_data < 750)) {
    return 5;
  } else {
    return 0;
  }
}

