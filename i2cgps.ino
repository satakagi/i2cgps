/*
 * Yet Another I2C GPS firmware with TinyGPSPlus
 *
 * TinyGPSPlusライブラリを使用してGPSデータを解析し、
 * I2C経由でデータを送信するファームウェアです。
 * NeoSWSerialでソフトウェアシリアル通信を行います。
 *
 * 【変更点】
 * ・使用ライブラリをNeoGPSからTinyGPSPlusに変更。(日付がおかしかった)
 * ・ライブラリのバージョン互換性のため、基本的な機能に絞って実装。
 * ・I2C送信時の緯度・経度データの計算方法を修正し、精度を向上。
 */

#include <Wire.h>
#include <WireData.h>
#include <NeoSWSerial.h>
#include <TinyGPSPlus.h> // TinyGPSPlusライブラリをインクルード

// GPSモジュールのボーレート
const long GPS_BAUD_RATE = 9600;

// PCのシリアルモニターのボーレート
const long PC_BAUD_RATE = 57600;

// I2Cアドレス
const int i2c_addr = 0x58;

// GPSモジュールのRXピンとTXピンを設定
NeoSWSerial gpsPort(10, 19); 

// TinyGPSPlusオブジェクトの宣言
TinyGPSPlus gps;

// I2Cで送信するデータ構造体
typedef struct fix {
  uint8_t fixlen;
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
  i2c_fix.fixlen = sizeof(i2c_fix);
  
  // TinyGPSPlusのデータで構造体を埋める
  i2c_fix.status = (uint8_t)gps.location.isValid(); // isValid()を使用
  
  // 緯度・経度データをrawLat/rawLngのdegとbillionthsから計算
  // 10^-7の単位で整数として送信
  RawDegrees latRaw = gps.location.rawLat();
  RawDegrees lngRaw = gps.location.rawLng();
  i2c_fix.lat = latRaw.deg * 10000000L + latRaw.billionths / 100L;
  i2c_fix.lon = lngRaw.deg * 10000000L + lngRaw.billionths / 100L;
  if (latRaw.negative) i2c_fix.lat = -i2c_fix.lat;
  if (lngRaw.negative) i2c_fix.lon = -i2c_fix.lon;

  i2c_fix.alt = gps.altitude.meters() * 100; // 高度をメートルからセンチメートルに変換
  i2c_fix.speed = gps.speed.kmph() * 10; // km/hの10倍として速度を取得

  // TinyGPSPlusから日時情報を取得
  i2c_fix.year = gps.date.year();
  i2c_fix.month = gps.date.month();
  i2c_fix.day = gps.date.day();
  i2c_fix.hour = gps.time.hour();
  i2c_fix.minute = gps.time.minute();
  i2c_fix.second = gps.time.second();
  
  // I2C経由で構造体を送信
  wireWriteData(i2c_fix);
}


void setup() {
  Serial.begin(PC_BAUD_RATE);
  Serial.println("Arduino Pro Mini has started.");
  Serial.println("Waiting for GPS data...");
  
  gpsPort.begin(GPS_BAUD_RATE);
  
  i2c_fix.fixlen = sizeof(i2c_fix);
  Wire.begin(i2c_addr);
  Wire.onRequest(i2c_position_request);
  
  pinMode(LED_BUILTIN, OUTPUT);
}


void loop() {
  // GPSポートにデータが届いていれば、
  // TinyGPSPlusでデータを解析する
  while (gpsPort.available()) {
    gps.encode(gpsPort.read());
  }
  
  // 新しいfixが取得できたら、シリアル出力
  if (gps.location.isUpdated()) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    
    Serial.println("--------------------");
    Serial.print("Status: ");
    Serial.println(gps.location.isValid()); // isValid()を出力
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 7);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 7);
    Serial.print("Date: ");
    Serial.print(gps.date.year());
    Serial.print("/");
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.print(gps.date.day());
    Serial.print(" ");
    Serial.print(gps.time.hour());
    Serial.print(":");
    Serial.print(gps.time.minute());
    Serial.print(":");
    Serial.println(gps.time.second());
    
    // DOP情報をシリアル出力に追加
    Serial.print("HDOP: ");
    Serial.println(gps.hdop.value() / 100.0, 2);
    
    Serial.println("--------------------");
  }
}
