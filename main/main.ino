/**
* @file main.ino
* @description This program is aimed at controlling the temperature of customers and the capacity of the store, opening the door to allowed customers. It prevents clutter in front of the store by tracking the store capacity from MIT App Inventor.
* @date 27.12.2020
* @author Alperen Derin
*/

#include <SPI.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MLX90614.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <ArduinoJson.h>
#include <Servo.h>
#include <string.h>

static const int OUTDOOR_SENSOR = D5;
static const int INDOOR_SENSOR = D6;
static const int BUZZER = D7;
static const int SERVO = D8;

static int PersonCount = 0;
static int PersonCapacity = 2;

#define FIREBASE_HOST "pandemicdoor-default-rtdb.firebaseio.com"     // the project name address from firebase id
#define FIREBASE_AUTH "GrhbdqL7zRDonZ79P3z81Exe0yKzScaqys7IpTtU"     // the secret key generated from firebase

#define WIFI_SSID "Your_Internet_SSID"             // input your home or public wifi name 
#define WIFI_PASSWORD "Your_Internet_Password"     // password of wifi ssid

#define SCREEN_WIDTH 128 	// OLED display width, in pixels
#define SCREEN_HEIGHT 32 	// OLED display height, in pixels
#define OLED_RESET    -1 	// Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); 	//graphic settings
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

Servo servo;
WiFiClient client;

void setup() {
  Serial.begin(9600);
    
  pinMode(OUTDOOR_SENSOR, INPUT);
  pinMode(INDOOR_SENSOR, INPUT);
  pinMode(BUZZER,OUTPUT);
  pinMode(SERVO, OUTPUT);
    
  mlx.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 	// the display usually communicates with the i2c protocol over the 0x3C port.
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);  								                      // don't proceed, display loop forever
  }
  display.setTextColor(SSD1306_WHITE);
  servo.attach(SERVO);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);             // try to connect with wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); 
    Serial.print(".");
  }
  
  Serial.println("\n----------------------------");
  Serial.println("WiFi connected");
  Serial.println("\n----------------------------");
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);     // connect to firebase
  if(Firebase.failed())
    Serial.println("Firebase Hatasi");
  else
    Serial.println("Firebase Basarili");

  PersonCapacity = Firebase.getInt("Sakarya_Merkez_Mudo/5401_Serdivan_Merkez_Mudo_Kisi_Kapasitesi"); // I got the customer capacity of the store directly from the database so that the number of contacts can be easily changed from Firebase if needed without making changes to the device
  PersonCount = Firebase.getInt("Sakarya_Merkez_Mudo/5401_Serdivan_Merkez_Mudo_Kisi_Sayisi");
}

void loop() {

  MainScreen();
  if (digitalRead(OUTDOOR_SENSOR) == LOW) { // If the outside distance sensor is triggered
    if(PersonCountControl() == false){      // The store is full of customers
      FloatTextOnDisplay("Malesef Iceride Musteri Kapasitemiz Doldu, Sizi Birazcik Bekletecegiz", 80, 8, 2);
    }
    else{ // The temperature should be checked to bring the person inside
      FloatTextOnDisplay("Lutfen Cihazin Tam Onune Gecerek Cikan Ses Boyunca Basinizi Sensore Yaklastiriniz", 80, 8, 2);
      tone(BUZZER,261);   // 'Do tone' comes from Buzzer
      delay(478);         
      noTone(BUZZER);
      delay(478);
      tone(BUZZER,261);
      delay(478);
      noTone(BUZZER);
      delay(478);

      if(digitalRead(INDOOR_SENSOR)== LOW && digitalRead(OUTDOOR_SENSOR) == LOW){  // If the sensor inside works after about 2 seconds, it means that the person is trying to get inside.
        int temp = mlx.readObjectTempC(); // Let's measure the temperature of the person while the sound is playing
        tone(BUZZER,261);
        delay(478);
        noTone(BUZZER);
        delay(478);

        if(ControlTemp(temp)){  // the person can go inside.
           tone(BUZZER,261);
           delay(856);
           noTone(BUZZER);
           DisplayTemprature(temp);
           servo.write(120);
           FloatTextOnDisplay("Iyi Alisverisler Dileriz...", 80, 8, 2);
           servo.write(90);
           PersonCount++;
           Firebase.setInt("Sakarya_Merkez_Mudo/5401_Serdivan_Merkez_Mudo_Kisi_Sayisi", PersonCount);
           MainScreen();
           Serial.print("Kisi Sayisi Güncellendi:"); Serial.println(PersonCount);

          // Check to close the door
          while(digitalRead(INDOOR_SENSOR == LOW)){ }  // As long as the sensor is working, it will cycle
          delay(3000);                                 // Wait 2 seconds after the sensor stops and drops
          if(digitalRead(INDOOR_SENSOR == HIGH)){      // If the sensor still does not work, close the door
            servo.write(60);
            delay(500);
            servo.write(90);
          }
        }
        else { // The person has a high fever
          DisplayTemprature(temp);
          FloatTextOnDisplay("Atesiniz Biraz Yuksek, Lutfen En Yakin Saglik Merkezine Basvurunuz", 80, 8, 2);
        }
      }
      else{ // It means that the person is not standing in front of the device's near-inside sensor, that is, she does not want to go inside.
        tone(BUZZER,261);delay(478); // Do tone
        noTone(BUZZER);delay(478);
        tone(BUZZER,493);delay(1014); // Si tone
        noTone(BUZZER);
        FloatTextOnDisplay("Cihazin Onunde Bulunmadiginizdan Islem Iptal Edilmistir", 80, 8, 2);
        Serial.println("Kisi Sayisi: "); Serial.println(PersonCount);
       }
    }
  }
  else if (digitalRead(INDOOR_SENSOR) == LOW) {
      PersonCount--; 
      Firebase.setInt("Sakarya_Merkez_Mudo/5401_Serdivan_Merkez_Mudo_Kisi_Sayisi", PersonCount);
      Serial.println("Kisi Sayisi: "); Serial.println(PersonCount);
      FloatTextOnDisplay("Iyi gunler dileriz..", 80, 8, 2);
      MainScreen(); 
  }  
}
void MainScreen(){
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.println("Hos Geldiniz...");
  display.println("---------------------");
  display.print("Musteri Sayisi: "); display.println(PersonCount);
  display.print("Musteri Kapasitesi: "); display.print(PersonCapacity);
  display.display();
}

boolean PersonCountControl(){
  if(PersonCount >= PersonCapacity){
    return false;
  }
  return true;
}

boolean ControlTemp(int temprature){
  if(temprature >= 33){
    return false;
  }
  else{
    return true;
  }
}

void DisplayTemprature(float temprature) {
  display.clearDisplay();
  display.setTextSize(2);       
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);

  display.println("Atesiniz:");
  display.print(temprature, 1);
  display.print((char)247);
  display.print("C");
  display.display();
  delay(2000);
}

void FloatTextOnDisplay(String message, int delayTime, int cursorHeight, int textSize){ // Since I have a small display with a resolution of 128 * 32, I want to scroll the texts on the OLED Display.
  int messageLength = message.length();
  String temp;
  display.setTextSize(textSize);
  display.setCursor(0,cursorHeight);
  
  for(int i = 0; i <= messageLength; i++){
    temp = message.substring(0+i, 10+i);
    display.clearDisplay();
    display.setCursor(0,cursorHeight);
    display.print(temp);
    display.display();
    delay(delayTime);
  }
}
