#include <SoftwareSerial.h>
#include "secrets.h"
SoftwareSerial sim800lSerial(3, 1);  // RX, TX

const int ledPin = D2;  // Change this to the pin where your LED is connectedc:\Users\jtkyr\development\Arduino\cabin_warmer_9000\.gitignore

bool sendATCommand(const String& command, const String& expectedResponse = "OK") {
  Serial.print("sending command: ");
  Serial.println(command);
  sim800lSerial.println(command);
  return checkResponse(expectedResponse);
}

bool checkResponse(const String& expectedReponse) {
  unsigned long startTime = millis();
  unsigned long timeout = 1000;
  while (millis() - startTime < timeout) {
    if (sim800lSerial.available()) {
      String response = sim800lSerial.readString();
      Serial.println(response);
      if (response.equals(expectedReponse)) {
        return true;
      } else {
        return false;
      }
    }
  }
  return false;
}

bool processSms(const String& smsContent) {
  int start_number = smsContent.indexOf("+CMT: \"") + 7;
  int end_number = smsContent.indexOf("\"", start_number);
  String phoneNumber = smsContent.substring(start_number, end_number);

  int start_message = smsContent.lastIndexOf("\"") + 1;
  String messageText = smsContent.substring(start_message);
  messageText.trim();
  if (phoneNumberIsApproved(phoneNumber)) {
    Serial.print("Received SMS from: " + phoneNumber);
    Serial.println(" - " + messageText);
  }
  return false;
}

bool phoneNumberIsApproved(const String& phoneNumber) {
  int size = sizeof(approvedPhoneNumbers) / sizeof(approvedPhoneNumbers[0]);
  for (int i = 0; i < size; i++) {
    if (phoneNumber.equals(approvedPhoneNumbers[i])) {
      return true;
    }
  }
  return false;
}

void sendSMS(const String& phoneNumber, const String& messageText) {
  if (sendATCommand("AT+CMGF=1;")) {
    if (sendATCommand("AT+CMGS=\"" + phoneNumber + "\"\r", ">")) {
      sim800lSerial.print(messageText);
      sim800lSerial.write(0x1A);  // Send CTRL+Z character to end message
      if (checkResponse("+CMGS:")) {
        Serial.println("SMS sent successfully");
        return;
      }
    }
  }
  Serial.println("Error: Failed to send SMS");
}

void setup() {
  sim800lSerial.begin(9600);
  Serial.begin(19200);

  pinMode(ledPin, OUTPUT);

  Serial.println("Initialzing...");
  Serial.println(sendATCommand("AT"));  // Check if board is up
  delay(1000);
  Serial.println(sendATCommand("AT+CMGF=1"));  // Set SMS mode to text
  delay(1000);
  Serial.println(sendATCommand("AT+CNMI=2,2,0,0,0"));  // Enable receiving SMS notifications
  delay(1000);
  Serial.println("Setup complete.");
}

void loop() {
  if (sim800lSerial.available() > 0) {
    String response = sim800lSerial.readString();
    if (response.indexOf("+CMT:") != -1) {
      processSms(response);
    }
  }
}
