#include <SoftwareSerial.h>
#include "secrets.h"
SoftwareSerial SIM7670Serial(5, 4);  // RX, TX

const int ledPin = D2;  // Change this to the pin where your LED is connectedc:\Users\jtkyr\development\Arduino\cabin_warmer_9000\.gitignore

bool sendATCommand(const char* cmd, const char* expectedResponse, unsigned long timeout) {
  Serial.print("Sending: ");
  Serial.print(cmd);
  Serial.println("...");
  SIM7670Serial.println(cmd);
  String response = "";
  unsigned long startTime = millis();
  bool responseOK = false;

  while (millis() - startTime < timeout) {
    while (SIM7670Serial.available() > 0) {
      char c = SIM7670Serial.read();
      response += c;
    }

    if (response.indexOf(expectedResponse) != -1) {
      responseOK = true;
      Serial.println(response);
      break;  // found it
    }
  }

  Serial.print("Response was: ");
  Serial.println(response);

  if (responseOK)
    Serial.println("Response OK");
  else
    Serial.println("Timeout without expected Response");
  return responseOK;
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
    return true;
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

void sendSMS(String number, String message) {
  String cmd = "AT+CMGS=\"" + number + "\"\r\n";
  SIM7670Serial.print(cmd);
  delay(100);
  SIM7670Serial.println(message);
  delay(100);
  SIM7670Serial.write(0x1A);  // send ctrl-z
  delay(100);
}

void setup() {
  SIM7670Serial.begin(115200);
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  sendATCommand("ATE0", "OK",1000);
  bool boardIsUp = sendATCommand("AT", "OK", 1000);
  while (!boardIsUp) {
    delay(500);
    Serial.println("Retrying..");
    boardIsUp = sendATCommand("AT", "OK", 1000);
  }

  sendATCommand("AT+CMGF=1", "OK", 1000);          // set SMS format to text
  sendATCommand("AT+CNMI=2,2,0,0,0", "OK", 1000);  // Enable receiving SMS notifications
  Serial.println("Setup complete.");

  sendSMS("+4797140645", "Hello from Arduino!");
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
