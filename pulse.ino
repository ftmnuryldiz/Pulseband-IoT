//Yinelenen işaretleri kullanmak için Ticker kütüphanesini ekliyorum.
#include <Ticker.h>

//Mikrodenetleyici ileyişimi için:
#include <SPI.h>

// Oled ekran I2C protokolüyle haberleşiyor: 
#include <Wire.h>

//Font ve grafik işlemleri için:
#include <Adafruit_GFX.h>

//Grafiği sürmek için:
#include <Adafruit_SSD1306.h>

 //Ekranım 128x64 grafik lcd ekran olduğu için girişlerini yapıyorum
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 

//Ekranımın uzunluğu ve genişliği için değişken tanımlıyorum. (sabit diye const kullandım.)
const int WIDTH = 128 ; 
const int HEIGHT = 64 ;
const int LENGTH = WIDTH;

 // 128x64 olsaydı 0x3C olurdu adresi. 
#define SCREEN_ADDRESS 0x3D

//Reset pinini yazdım. 
#define OLED_RESET 0  

//I2C'yi kullandığı için ekranımızı belli ederken &Wire kullanıyoruz.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Kütüphane dosyaları:
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

//Wifi bağlantısı için:
#define WLAN_SSID "karakahya"
#define WLAN_PASS "U12323232"

//Adafruit setup için
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME "fatmanuryildizz"
#define AIO_KEY "23232"

//WiFi bağlantı nesnesi oluşturma:
WiFiClient client;

//Adafruit MQTT bağlantısı 
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

//Adafruit feedleri
Adafruit_MQTT_Publish pulse = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/pulse");

 int rate[10];      
 unsigned long sampleCounter = 0; 
 unsigned long lastBeatTime = 0;  
 int P =512;            
 int T = 512;    
 int thresh = 512;         
 int amp = 100;                  
 boolean firstBeat = true;       
 boolean secondBeat = false;  
 int BPM;       
 int Signal;    
 int IBI = 600;             
 boolean Pulse = false;    
 boolean QS = false;     

Ticker flipper;

int fadePin = 12;                 
int fadeRate = 0;                 

int x;
int y[LENGTH];

void MQTT_connect();

void setup(){
   Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  
  delay(20);
  
 Serial.println(F("Adafruit IO Example"));
WiFi.begin(WLAN_SSID, WLAN_PASS);
while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }

  display.clearDisplay();
  
  x = 0;
  clearY();
  pinMode(fadePin,OUTPUT);         
           
  interruptSetup(); 
  
}

void MQTT_connect(){
int8_t ret;
if(mqtt.connected())
  {
    return;
   }
    Serial.print("MQTT'ye bağlanılıyor...");
    
   uint8_t retries = 3;
 while ((ret = mqtt.connect()) != 0)
 { 
 Serial.println(mqtt.connectErrorString(ret));
 Serial.println("Tekrar deneniyor.");
 mqtt.disconnect();
 delay(5000); 
 retries--;
 
 if (retries == 0)
 {
 while (1) ;
 }
 }
 Serial.println("MQTT'ye bağlandım.");
 }

void clearY(){
  for(int i=0; i<=LENGTH; i++){
    y[i] = -1;
  }
}

void drawY(){
  display.drawPixel(0, y[0], WHITE);
  for(int i=1; i<=LENGTH; i++){
    if(y[i]!=-1){
      display.drawLine(i-1, y[i-1], i, y[i], WHITE);
    }else{
      break;
    }
  }
}

void loop(){
 MQTT_connect();
  y[x] = map(Signal, 0, 1023, HEIGHT-14, 0); 
    drawY();
  x++;
  if(x >= WIDTH){

        display.clearDisplay();
        display.drawLine(0, 51, 127, 51, WHITE);
        display.drawLine(0, 63, 127, 63, WHITE);
        display.setTextSize(0);
        display.setTextColor(WHITE);
        display.setCursor(0,54);
        display.print(" nabiz = ");
        display.print(BPM);
        display.print("  va = ");
        display.print(IBI);
        display.print("  ");
    x = 0;
    clearY();
    
    pulse.publish(BPM);
    if (! pulse.publish(BPM)) {                     
      Serial.println(F("Gönderemedik."));
    } 
      
    else {
      Serial.println(F("Gönderdik."));
    }
  }


  sendDataToProcessing('S', Signal); 
  if (QS == true){                      
        fadeRate = 255;
                     
        sendDataToProcessing('B',BPM);
  
        sendDataToProcessing('Q',IBI);   
        QS = false;      

     }
     
  display.display();   
  delay(10);                             
}

void sendDataToProcessing(char symbol, int data ){
    Serial.print(symbol); 
    Serial.println(data);                
  }


void interruptSetup(){     
  flipper.attach_ms(2, ISRTr);     
} 



void ISRTr(){                          
  cli();                         
  Signal = analogRead(A0);        
  sampleCounter += 2;    

  int N = sampleCounter - lastBeatTime;  


  if(Signal < thresh && N > (IBI/5)*3){   
    if (Signal < T){                        
      T = Signal;                        
    }
  }

  if(Signal > thresh && Signal > P){  
    P = Signal;       
  }                                                 


  if (N > 250){               
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) ){        
      Pulse = true;                               
      IBI = sampleCounter - lastBeatTime;         
      lastBeatTime = sampleCounter;              

      if(secondBeat){          
        secondBeat = false;                  
        for(int i=0; i<=9; i++){          
          rate[i] = IBI;                      
        }
      }

      if(firstBeat){          
        firstBeat = false;                   
        secondBeat = true;                   
        sei();                
        return;                              
      }   


     
      word runningTotal = 0;                      

      for(int i=0; i<=8; i++){              
        rate[i] = rate[i+1];                  
        runningTotal += rate[i];             
      }

      rate[9] = IBI;                          
      runningTotal += rate[9];                
      runningTotal /= 10;                     
      BPM = 60000/runningTotal;               
      QS = true;                              
      
    }                       
  }

  if (Signal < thresh && Pulse == true){   
    Pulse = false;                         
    amp = P - T;                           
    thresh = amp/2 + T;      
    P = thresh;                            
    T = thresh;
  }

  if (N > 2500){             
    thresh = 512;            
    P = 512;                               
    T = 512;                               
    lastBeatTime = sampleCounter;                
    firstBeat = true;                      
    secondBeat = false;                    
  }

  sei();                
} 
