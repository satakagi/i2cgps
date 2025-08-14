/*
 * Yet Another I2C GPS firmware.
 * 
 * Because we have a whole microcontroller with ram 'n' stuff, it is possible
 * to parse the data on the AVR, and just send back a struct over i2c
 *
 * On the Wemos D1 Mini/Micropython end:
 *    SCL = D1 / GPIO5 / Pin(5) / phy_pin(20)
 *    SDA = D2 / GPIO4 / Pin(4) / phy_pin(14)
 * 
 * On the OSEPP Pro Mini, there are two extra holes for the I2C port. A4 (SDA)
 * is closer to the AVR, and A5 (SCL) A5 is closer to the FTDI header. Arduino
 * calls those pins 27 and 28 and the Wire library just assumes it can use them.
 *
 */
/*
 * Yet Another I2C GPS firmware
 *
 * NeoGPSでGPSデータを解析し、I2C経由でデータを送信します。
 * NeoSWSerialを使用して、ソフトウェアシリアル通信を行います。
 */

#include <Wire.h>
#include <WireData.h> // I2Cで構造体を送信するためのライブラリ

// ソフトウェアシリアルライブラリをインクルード
#include <NeoSWSerial.h>
#include <NMEAGPS.h>

// GPSモジュールのボーレート
const long GPS_BAUD_RATE = 9600;

// PCのシリアルモニターのボーレート
const long PC_BAUD_RATE = 57600;

// I2Cアドレス
const int i2c_addr = 0x58; // 'X' marks the spot.

// GPSモジュールのRXピンとTXピンを設定
// TXピン(19)は未使用
NeoSWSerial gpsPort(10, 19); 

// NeoGPSオブジェクトの宣言
NMEAGPS gps;
gps_fix fix;

// I2Cで送信するデータ構造体
typedef struct fix {
  uint8_t fixlen; // 構造体のサイズ
  uint8_t status;
  uint16_t year;
  uint8_t month, day, hour, minute, second;
  int32_t lat, lon;
  int32_t alt;
  uint32_t speed;
} Fix_t;

// 送信するデータ構造体
static Fix_t i2c_fix;

// I2Cリクエストハンドラ
void i2c_position_request() {
  i2c_fix.status = fix.status;
  i2c_fix.lat = fix.latitudeL();
  i2c_fix.lon = fix.longitudeL();
  i2c_fix.alt = fix.altitude_cm();
  i2c_fix.speed = fix.speed_mkn();
  i2c_fix.year = fix.dateTime.year;
  i2c_fix.month = fix.dateTime.month;
  i2c_fix.day = fix.dateTime.day;
  i2c_fix.hour = fix.dateTime.hours;
  i2c_fix.minute = fix.dateTime.minutes;
  i2c_fix.second = fix.dateTime.seconds;
  
  // I2C経由で構造体を送信
  wireWriteData(i2c_fix);
}


void setup() {
  // PCとのシリアル通信を開始
  Serial.begin(PC_BAUD_RATE);
  Serial.println("Arduino Pro Mini has started.");
  Serial.println("Waiting for GPS data...");
  
  // GPSモジュールとの通信を開始
  gpsPort.begin(GPS_BAUD_RATE);
  
  // I2C通信を開始
  i2c_fix.fixlen = sizeof(i2c_fix);
  Wire.begin(i2c_addr);
  Wire.onRequest(i2c_position_request);
  
  pinMode(LED_BUILTIN, OUTPUT);
}


void loop() {
  // GPSポートにデータが届いていれば、
  // NeoGPSでデータを解析する
  if (gps.available(gpsPort)) {
    // 解析済みのデータを取得
    fix = gps.read();
    
    // LEDを点滅させて、正常に受信・解析されていることを示す
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    
    // デバッグ用のシリアル出力
    Serial.println("--------------------");
    Serial.print("Status: ");
    Serial.println(fix.status);
    Serial.print("Latitude: ");
    Serial.println(fix.latitude(), 7);
    Serial.print("Longitude: ");
    Serial.println(fix.longitude(), 7);
    Serial.print("Date: ");
    Serial.print(fix.dateTime.year);
    Serial.print("/");
    Serial.print(fix.dateTime.month);
    Serial.print("/");
    Serial.print(fix.dateTime.day);
    Serial.print(" ");
    Serial.print(fix.dateTime.hours);
    Serial.print(":");
    Serial.print(fix.dateTime.minutes);
    Serial.print(":");
    Serial.println(fix.dateTime.seconds);
    Serial.println("--------------------");
  }
}
