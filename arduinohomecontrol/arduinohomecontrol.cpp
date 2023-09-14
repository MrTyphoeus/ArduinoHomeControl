
// Libraries
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <MFRC522.h>
#include <Wire.h>

// Pin Definitions
#define DHT_PIN 2
#define LED_PIN 3 
#define BUZ_PIN 5
#define ECHO_PIN 6
#define TRIG_PIN 7
#define BTN_PIN 8
#define RST_PIN 9
#define SS_PIN 10
#define POT_PIN A0

// Constants
#define DHTTYPE DHT11
#define DISTANCE_THRESHOLD 50 // Threshold value for detected distance

// Global Variables
long distance;
int potValue = 0;
int delayHolder = 0;
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

  lcd.begin();      // Initialize LCD
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
  distance = DistanceCalculator(&distance);

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
  SerialPrinter(humidityValue, temperatureValue, potValue, isClose, isBuzzed);

  // Print values to LCD
  lcdPrinter(humidityValue, temperatureValue, potValue, isBuzzed);

  // Reads RFID module
  RFIDScanner(&isCard);


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
    delayHolder = 1;
    *isBuzzed = 1;
    tone(BUZ_PIN, 450); // Start the buzzer at 450 Hz frequency
  }

  if (delayHolder > 0 && delayHolder < 8){
    delayHolder++;
  }

  else if (delayHolder == 8){
    delayHolder++;
    noTone(BUZ_PIN);
  }

  else if (delayHolder > 8 && delayHolder < 30) delayHolder++;

  else {
    delayHolder=0;
    isBuzzed = 0;
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
  Serial.println(isCard); // Print buzzer status
}

void lcdPrinter(int humidityValue, int temperatureValue, int potValue,int isBuzzed){

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hu: ");
  lcd.print(humidityValue); // Display humidity
  lcd.setCursor(8, 0);
  lcd.print(" C: ");
  lcd.print(temperatureValue); // Display temperature
  lcd.setCursor(0, 1);

  if (isBuzzed)
    lcd.print("Doorbell Pressed"); // Display doorbell status
}

void RFIDScanner(int *isCard){
   // Look for new cards

  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Show UID on serial monitor

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  content.toUpperCase();
  if (content.substring(1) == "E7 EC EA D8") // Change here the UID of the card/cards that you want to give access
  {
    *isCard = 1;
  } else { 
    *isCard = 2;
  }
  delay(3000);
}
