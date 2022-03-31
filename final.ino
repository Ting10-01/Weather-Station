#include <Arduino_FreeRTOS.h>
#include <dht.h>
#include <LiquidCrystal_I2C.h>

dht DHT;
LiquidCrystal_I2C lcd(0x3F, 16, 2);

int DHT11pin = A0;
int echo = A1;
int trig = 2;
int photoresister = A2;
int sun[2] = {5, 6};
int cloud = 3;
int rain[5] = {4, 7, 8, 12, 13};

bool Update = true;;
int second=0, minute=0;
int distance = 0;
char Tvalue[3], Bvalue[3], Hvalue[3];

void setup() {
  Serial.begin(9600);
  pinMode(echo, INPUT);
  pinMode(trig, OUTPUT);
  pinMode(photoresister, INPUT);
  pinMode(sun[0], OUTPUT);
  pinMode(sun[1], OUTPUT);
  pinMode(cloud, OUTPUT);
  for(int i=0; i<5; i++)
    pinMode(rain[i], OUTPUT);

  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, HIGH);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T");
  lcd.setCursor(9, 0);
  lcd.print("H");
  lcd.setCursor(0, 1);
  lcd.print("B");
  
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  TCCR1B |= (1<<WGM12);
  TCCR1B |= (1<<CS12) | (1<<CS10);
  OCR1A = 15624;
  TIMSK1 |= (1<<OCIE1A);
  sei();
  
  xTaskCreate(getDistance, "getDistance", 128, NULL, 1, NULL);
  xTaskCreate(Display, "Display", 128, NULL, 1, NULL);
  vTaskStartScheduler();
}

void rest(){
  analogWrite(sun[0], 0);
  analogWrite(sun[1], 0);
  analogWrite(cloud, 0);
}

void Sun(int temp){
  analogWrite(sun[0], (float)temp/35*255);
  analogWrite(sun[1], (1-(float)temp/35)*255);
}

void Cloud(int bright){
  if(bright < 100)
    analogWrite(cloud, (300-bright)/10);
  else
    analogWrite(cloud, 0);
}

void Rain(int humi){
  //if(humi > 50){
    for(int i=0; i<5; i++){
      digitalWrite(rain[i], HIGH);
      delay((100-humi)*2);
      digitalWrite(rain[i], LOW);
    }
  //}
}

void getDistance(void *pvParameters){
  (void) pvParameters;
  for(;;){
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, HIGH);
    distance = pulseIn(echo, HIGH)*0.034/2;
    delay(10);
  }
}

void Display(void *pvParameters){
  (void) pvParameters;
  for(;;){
    if(distance > 20)
      rest();
    else{
      if(Update){
        int temp = DHT.read11(DHT11pin);
        Update = false;
        int t = DHT.temperature;
        int h = DHT.humidity;
        sprintf(Tvalue, "%d", t);
        lcd.setCursor(2, 0);
        lcd.print(Tvalue[0]);
        lcd.setCursor(3, 0);
        lcd.print(Tvalue[1]);
        sprintf(Hvalue, "%d", h);
        lcd.setCursor(11, 0);
        lcd.print(Hvalue[0]);
        lcd.setCursor(12, 0);
        lcd.print(Hvalue[1]);
      }
      Sun(DHT.temperature);
      //Sun(35);
      int bright = analogRead(photoresister);
      Cloud(bright);
      sprintf(Bvalue, "%d", bright);
      lcd.setCursor(2, 1);
      lcd.print(Bvalue[0]);
      lcd.setCursor(3, 1);
      lcd.print(Bvalue[1]);
      lcd.setCursor(4, 1);
      if(bright > 100)
        lcd.print(Bvalue[2]);
      else
        lcd.print(" ");
      Rain(DHT.humidity);
    }
    delay(1);
  }
}

void loop() {}

ISR(TIMER1_COMPA_vect){
  second++;
  //if(second == 60){
  if(second == 5){
    second = 0;
    minute++;
    //if(minute == 60){
    if(minute == 5){
      minute = 0;
      Update = true;
    }
  }
}
