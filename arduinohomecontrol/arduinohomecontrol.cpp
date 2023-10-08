
// Libraries
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Servo.h>

// Pin Definitions
#define DHT_PIN 2
#define LED_PIN 9 
#define BUZ_PIN 4
#define ECHO_PIN 6
#define TRIG_PIN 7
#define BTN_PIN 8
#define RST_PIN 3
#define SS_PIN 10
#define POT_PIN A0

// Constants
#define DHTTYPE DHT11
#define DISTANCE_THRESHOLD 50 // Threshold value for detected distance

// Global Variables
Servo servoMotor;
long distance;
bool openDoor = false;
int potValue = 0;
int buzzerDelayHolder = 0;
int rfidDelayHolder = 0;
int isClose = 0;
int isBuzzed = 0;
int isCard = 0;

DHT dht(DHT_PIN, DHTTYPE);
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {

  Serial.begin(9600);   // Initialize serial communication

  SPI.begin();      // Initialize SPI bus

  dht.begin();      // Initialize DHT sensor

  lcd.init();      // Initialize LCD
  lcd.backlight();  // Turn on the backlight of the LCD screen

  mfrc522.PCD_Init();   // Initialize MFRC522

  pinMode(LED_PIN, OUTPUT); // Set LED pin as output

  pinMode(TRIG_PIN, OUTPUT); // Set trigger pin for distance measurement
  pinMode(ECHO_PIN, INPUT);  // Set echo pin for distance measurement

  pinMode(BUZ_PIN, OUTPUT); // Set the buzzer pin as an OUTPUT
  pinMode(BTN_PIN, INPUT);  // Set button pin as input

  Serial.println();  // Print a blank line to serial monitor
}

void loop() {  

  // Distance calculator
  DistanceCalculator(&distance);

  isClose = (distance < DISTANCE_THRESHOLD) ? 1 : 0;
 
  // Potentiometer value reader
  potValue = analogRead(POT_PIN);

  // DHT11 value reader
  int humidityValue = dht.readHumidity();
  int temperatureValue = dht.readTemperature();

  // Button state value reader
  int buttonState = digitalRead(BTN_PIN);

  
  // Adjust LED brightness and potentiometer value according to potentiometer value
  potValue = map(potValue, 0, 760, 0, 10);
  int ledBrightness = map(potValue, 0, 10, 0, 255);
  analogWrite(LED_PIN, ledBrightness);
  

  // Button and buzzed delay control
  BuzzerControl(buttonState, &isBuzzed);

  // Print values to serial monitor
  SerialPrinter(humidityValue, temperatureValue, potValue, isClose, isBuzzed, isCard);

  // Print values to LCD
  lcdPrinter(humidityValue, temperatureValue, potValue, isClose, isBuzzed, isCard);

  // Opens the door
  openTheDoor(&openDoor);

  // Reads RFID module
  RFIDScanner(&isCard, &openDoor);


  delay(80);  // Short delay to prevent rapid value changes

}

int DistanceCalculator(long *distance){

  long duration;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calculate the distance in cm
  *distance = (duration / 2) / 29.1; // Speed of sound is 29.1 µs/cm (may vary depending on surface) 
}

int BuzzerControl(int buttonState, int *isBuzzed){

    if (buttonState == HIGH) {
    buzzerDelayHolder = 1; 
    *isBuzzed = 1;
    tone(BUZ_PIN, 450); // Start the buzzer at 450 Hz frequency
  }

  if (buzzerDelayHolder > 0 && buzzerDelayHolder < 8){ 
    buzzerDelayHolder++; 
  }

  else if (buzzerDelayHolder == 8){ 
    buzzerDelayHolder++; 
    noTone(BUZ_PIN);
  }

  else if (buzzerDelayHolder > 8 && buzzerDelayHolder < 30) buzzerDelayHolder++; 

  else {
    buzzerDelayHolder=0; 
    *isBuzzed = 0;
  }

}

void SerialPrinter(int humidityValue, int temperatureValue, int potValue, int isClose, int isBuzzed, int isCard){
  Serial.print(humidityValue); // Print humidity
  Serial.print(F("/"));
  Serial.print(temperatureValue); // Print temperature in Celsius
  Serial.print(F("/"));
  Serial.print(potValue); // Print potentiometer value
  Serial.print(F("/"));
  Serial.print(isClose); // Print distance status
  Serial.print(F("/"));
  Serial.print(isBuzzed); // Print buzzer status
  Serial.print(F("/"));
  Serial.println(isCard); // Print card status
}

void lcdPrinter(int humidityValue, int temperatureValue, int potValue, int isClose, int isBuzzed, int isCard){

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hu: ");
  lcd.print(humidityValue); // Display humidity
  lcd.setCursor(10, 0);
  lcd.print(" C: ");
  lcd.print(temperatureValue); // Display temperature
  lcd.setCursor(0, 1);

  if (isClose)
    lcd.print(" Someone's near "); // Display distance sensor 

  if (isBuzzed)
    lcd.print("Doorbell Pressed"); // Display doorbell status

  if (isCard == 1)
    lcd.print("Access granted");  
  
  if (isCard == 2)
    lcd.print("Access denied");


}

void RFIDScanner(int *isCard, bool *openDoor){

  if (rfidDelayHolder > 0 && rfidDelayHolder < 10){ 
    rfidDelayHolder++; 
  }

  else if (rfidDelayHolder == 10){
    *isCard = 0; 
    rfidDelayHolder = 0;
  }



   // Look for new cards

  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  rfidDelayHolder = 1;


  String content = "";

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  content.toUpperCase();
  if (content.substring(1) == "E7 EC EA D8"){ // Change here the UID of the card/cards that you want to give access
    *openDoor = true;
    *isCard = 1;
  }

  else{
    *isCard = 2;
    tone(BUZ_PIN, 100);
    delay(150);
    noTone(BUZ_PIN);
  }
  
}

void openTheDoor(bool *openDoor){
  if(*openDoor == true){

    tone(BUZ_PIN, 100);
    delay(50);
    noTone(BUZ_PIN);

    servoMotor.attach(5);
      for (int aci = 0; aci <= 180; aci += 1) {  // 0 ile 180 derece arasında dönme
        servoMotor.write(aci);  // Açıyı ayarla
        delay(15);  // Bekleme süresi (gecikme)
      }
    delay(600);
     for (int aci = 180; aci >= 0; aci -= 1) {  // 0 ile 180 derece arasında dönme
        servoMotor.write(aci);  // Açıyı ayarla
        delay(15);  // Bekleme süresi (gecikme)
      }
    servoMotor.detach();

    *openDoor = false;

  }

}
