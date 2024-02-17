#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include "secrets.h"
SoftwareSerial SIM7670Serial(5, 4);  // RX, TX

const int txPin = D2;
const int relayPin = 16;

bool checkResponse(const String response, const char* expectedResponse) {
  bool responseOK = false;
  if (response.indexOf(expectedResponse) != -1) {
    responseOK = true;
  }

  if (responseOK) {
    Serial.println("Response OK");
  } else {
    Serial.println(response);
    Serial.println("Timeout without expected Response");
  }
  return responseOK;
}

String sendATCommand(const char* cmd, unsigned long timeout) {
  Serial.print("Sending: ");
  Serial.print(cmd);
  Serial.println("...");
  SIM7670Serial.println(cmd);
  String response = "";
  unsigned long startTime = millis();
  while (millis() - startTime < timeout) {
    while (SIM7670Serial.available() > 0) {
      char c = SIM7670Serial.read();
      response += c;
    }
  }
  return response;
}


void processSms(const String& smsContent) {
  int start_number = smsContent.indexOf("+CMT: \"") + 7;
  int end_number = smsContent.indexOf("\"", start_number);
  String phoneNumber = smsContent.substring(start_number, end_number);
  int start_message = smsContent.lastIndexOf("\"") + 1;
  String messageText = smsContent.substring(start_message);
  messageText.toLowerCase();
  messageText.trim();
  if (phoneNumberIsApproved(phoneNumber)) {
    Serial.print("Received SMS from: " + phoneNumber);
    Serial.println(" - " + messageText);
    if (messageText == "på" || messageText == "pÅ") {  // Å is not lowered by toLowerCase()
      heaterOn();
      sendSMS(phoneNumber, "On signal sent");
    } else if (messageText == "av") {
      heaterOff();
      sendSMS(phoneNumber, "Off signal sent");
    } else if (messageText == "temp" || messageText == "temperatur") {
      double temp = readTemperature();
      sendSMS(phoneNumber, String(temp));
    }
  }
}


void heaterOn() {
  Serial.println("Turning heater on");
  digitalWrite(relayPin, HIGH);
  delay(3000);
  digitalWrite(relayPin, LOW);
}

void heaterOff() {
  Serial.println("Turning heater off");
  digitalWrite(relayPin, HIGH);
  delay(5000);
  digitalWrite(relayPin, LOW);
}

double readTemperature() {
  // Not implemented yet
  return 20;
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

void sendSMS(String number, String message) {
  String cmd = "AT+CMGS=\"" + number + "\"\r\n";
  SIM7670Serial.print(cmd);
  delay(100);
  SIM7670Serial.print(message);
  delay(100);
  SIM7670Serial.write(0x1A);  // send ctrl-z
  delay(100);
}

void setup() {
  // Set up serial
  SIM7670Serial.begin(115200);
  Serial.begin(115200);

  // Turn off wifi to save power
  WiFiMode(WIFI_STA);
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);

  // Set up pins
  pinMode(txPin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  // Initialize A7670G
  sendATCommand("ATE0", 1000);
  bool boardIsUp = checkResponse(sendATCommand("AT", 1000), "OK");
  while (!boardIsUp) {
    delay(500);
    Serial.println("Retrying..");
    boardIsUp = checkResponse(sendATCommand("AT", 1000), "OK");
  }
  sendATCommand("AT+CMGF=1", 1000);          // set SMS format to text
  sendATCommand("AT+CNMI=2,2,0,0,0", 1000);  // Enable receiving SMS notifications
  Serial.println("Setup complete.");
}

void loop() {
  delay(1000);
  if (SIM7670Serial.available()) {
    String response = SIM7670Serial.readString();
    Serial.println(response);
    if (response.indexOf("+CMT:") != -1) {
      processSms(response);
    }
  }
}