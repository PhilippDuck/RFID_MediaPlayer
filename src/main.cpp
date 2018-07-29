#include <Arduino.h>
#include "SoftwareSerial.h"
#include <SPI.h>
#include <MFRC522.h>

SoftwareSerial softwareSerial(2, 3); // RX, TX

int blueLed       = 4; // RGBLed
int greenLed      = 5; // RGBLed (PWM)
int redLed        = 6; // RGBLed (PWM)
int playButton    = 7;
int playbuttonLed = 8;

#define SS_PIN    10
#define RST_PIN   9

String uid;

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key;

// Init array that will store new NUID
byte nuidPICC[4];

# define Start_Byte 0x7E
# define Version_Byte 0xFF
# define Command_Length 0x06
# define End_Byte 0xEF
# define Acknowledge 0x00 //Returns info with command 0x41 [0x01: info, 0x00: no info]


void execute_CMD(byte CMD, byte Par1, byte Par2) // Excecute the command and parameters
{
    // Calculate the checksum (2 bytes)
    int16_t checksum = -(Version_Byte + Command_Length + CMD + Acknowledge + Par1 + Par2);

    // Build the command line
    byte Command_line[10] = { Start_Byte, Version_Byte, Command_Length, CMD,
       Acknowledge, Par1, Par2, checksum >> 8, checksum & 0xFF, End_Byte};

    //Send the command line to the module
    for (byte k=0; k<10; k++)
    {
        softwareSerial.write( Command_line[k]);
    }
    delay(200);
}

void receiveMessage()
{
    int message[10];
    if (softwareSerial.available() >= 10)
    {
        // Anzahl der im Buffer enthaltenen Bytes.
        Serial.print("SoftwareSerialBuffer: ");
        Serial.println(softwareSerial.available());

        // Solange das Startbyte nicht gelesen wurde,
        // wird das aktuell gelesene Byte durch .read()
        // gelöscht.
        while (softwareSerial.read() != 126)
        {
            message[0] = 126;
        }

        // Nachdem das Startbyte "gefunden" wurde,
        // werden die nächsten 9 Bytes gelesen und in
        // dem Array message gespeichert.
        for (int i = 1; i < 10; i++)
        {
            message[i] = softwareSerial.read();
        }

        // Das Array wird auf dem Seriellen Monitor
        // formatiert ausgegeben.
        for (int i = 0; i < 10; i++)
        {
           Serial.print(message[i], HEX);
           Serial.print(" ");
        }
        Serial.println("");

        switch (message[3]) {
          case 0x4C:
              Serial.print("Aktueller Track: ");
              Serial.println(message[6]);
        }
    }
}

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize) {
  uid = "";
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
    uid += (buffer[i] < 0x10 ? " 0" : "");
    uid += (buffer[i]);
  }
  Serial.println("");
  Serial.println(uid);
}

void readUID(){
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
    rfid.uid.uidByte[1] != nuidPICC[1] ||
    rfid.uid.uidByte[2] != nuidPICC[2] ||
    rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }

    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
  }
  else Serial.println(F("Card read previously."));

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}


void setup() {
    pinMode(greenLed, OUTPUT);
    pinMode(redLed, OUTPUT);
    pinMode(blueLed, OUTPUT);
    pinMode(playbuttonLed, OUTPUT);
    pinMode(playButton, INPUT);


    //Schalte stateLed ein.
    analogWrite(greenLed, 10);

    //beginne Serielle Verbindungen
    Serial.begin(115200);
    softwareSerial.begin(9600);
    Serial.println("RFID_MediaPlayer gestartet.");

    // Reset Modul
    execute_CMD(0x0C, 0, 0);
    delay(500);
    // Lautstärke einstellen
    execute_CMD(0x06, 0, 10);
    // Playback
    //execute_CMD(0x0D, 0, 0);

    SPI.begin(); // Init SPI bus
    rfid.PCD_Init(); // Init MFRC522

    for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;
    }

    Serial.println(F("This code scan the MIFARE Classsic NUID."));
    Serial.print(F("Using the following key:"));
    printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
}

int buttonCount = 0;
bool button;
bool buttonWasPressed = false;
unsigned long currentTime;
unsigned long buttonStartTime;
float buttonTimePressed;

void loop()
{
    // Speichert aktuelle Zeit
    currentTime = millis();

    // wartet auf Nachrichten vom Serialbus
    receiveMessage();

    // ließt den Zustand des Buttons
    button = digitalRead(playButton);

    // verarbeitet Eingaben über den Button
    if (button and not buttonWasPressed)
    {
        delay(100);
        buttonCount += 1;
        buttonStartTime = millis(); // Startzeit des Button
        buttonWasPressed = true;
        Serial.print("Taster wird gedrückt. ");
        Serial.println(buttonCount);

    } else if (not button and buttonWasPressed)
    {
        buttonWasPressed = false;
        // Errechne Dauer der Tasterbetätigung
        buttonTimePressed = (currentTime - buttonStartTime);
        Serial.print("Taster losgelassen. Dauer: ");
        Serial.print(buttonTimePressed/1000);
        Serial.println(" Sekunden");

        if (buttonTimePressed < 1000)
        {
          execute_CMD(0x01, 0, 0); // Next track
          execute_CMD(0x4C, 0, 0);
        }
    }

    // Wartet auf Rfid-Karten
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){
      digitalWrite(blueLed, HIGH);
      delay(50);
      digitalWrite(blueLed, LOW);
      readUID();
    }

}
