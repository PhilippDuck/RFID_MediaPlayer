#include <Arduino.h>
#include "SoftwareSerial.h"

SoftwareSerial softwareSerial(2, 3); // RX, TX

int blueLed       = 4; // RGBLed
int greenLed      = 5; // RGBLed (PWM)
int redLed        = 6; // RGBLed (PWM)
int playButton    = 7;
int playbuttonLed = 8;

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
    }
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

}

int buttonCount = 0;
bool button;
bool buttonWasPressed = false;

void loop()
{
    receiveMessage();

    button = digitalRead(playButton);
    if (button and not buttonWasPressed)
    {
        delay(100);
        buttonCount += 1;
        buttonWasPressed = true;
        Serial.print("Taster wird gedrückt. ");
        Serial.println(buttonCount);
        execute_CMD(0x01, 0, 0); // Nächster Track
        execute_CMD(0x4C, 0, 0); // Frage nach aktuellem Track

    } else if (not button and buttonWasPressed)
    {
        buttonWasPressed = false;
    }
}
