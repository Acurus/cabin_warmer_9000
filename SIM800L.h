/*************************************************************************************
 *  Created By: Tauseef Ahmad
 *  Created On: 15 April, 2023
 *  
 *  Tutorial: https://youtu.be/lYN8hqNAKbU
 *  My Channel: https://www.youtube.com/channel/UCOXYfOHgu-C-UfGyDcu5sYw/
 ***********************************************************************************/
 
 #include <SoftwareSerial.h>

class SIM800L {
  //------------------------------------------------------------------------------------
  private:
    const SoftwareSerial simSerial;
    //Since the callback functions do not modify the phone number 
    //or message text, their parameters can be marked as const.
    void (*callCallback)(const String&);
    
    String phoneNumber;
    String messageText;
    int resetPin;
    bool debug = false;
  
  public:
    //Class Constructure
    SIM800L(int rxPin, int txPin, int resetPin = -1) : simSerial(rxPin, txPin), resetPin(resetPin) {}

    /*************************************************************************************************
     * The purpose of this function is to 
	 1. initialize a software serial interface
	 2. reset the module (if a reset pin is provided)
	 3. waits for the module to respond and checks if it is ready.
	 4. configure the SIM800L module for SMS and call functionality.
     *************************************************************************************************/
    void begin(unsigned long baudrate = 9600) {
      //---------------------------------------------------------------------------------
      // Initialize the software serial interface
      simSerial.begin(baudrate);
      pinMode(resetPin, OUTPUT);
      // Reset the module, if a reset pin was provided
      reset();
      //---------------------------------------------------------------------------------
      // Wait for the module to respond
      if (isModuleReady()) {
        //configure sim800l for sms and call
        tryATcommand("ATE1"); // Echo ON
        tryATcommand("AT+CMGF=1;"); // Set SMS mode to text
        // set the new message indication mode
        //tryATcommand("AT+CNMI=2,2,0,0,0"); //+CMTI: "SM", index
        tryATcommand("AT+CNMI=1,2,0,0,0"); //+CMT: "PHONE_NUMBER","","DateTime" MESSAGE
        tryATcommand("AT+CLIP=1"); // enable caller ID
        tryATcommand("AT+DDET=1"); // Enable DTMF detection
        Serial.println("Module is ready.");
      } else {
        Serial.println("Failed to initialize the module after multiple attempts.");
      }
    }
    
    /****************************************************************************************************
     * This function is to listen for incoming data from the SIM800L via the software serial interface.
     ****************************************************************************************************/
    void listen() {
      int availableBytes = simSerial.available();
      if (availableBytes <= 0) { return; }
      //while(simSerial.available()){
        String incomingResponse = simSerial.readString();
        if (debug) {
          Serial.println(incomingResponse);
        }
        //-------------------------------------------------------------
        // Check for incoming SMS Notifications
        if (incomingResponse.indexOf("+CMT:") != -1) {
          // We have received a new message
          int idx1 = incomingResponse.indexOf("+CMT: \"") + 7;
          int idx2 = incomingResponse.indexOf("\"", idx1);
          phoneNumber = incomingResponse.substring(idx1, idx2);
          
          idx1 = incomingResponse.lastIndexOf("\"") + 1;
          messageText = incomingResponse.substring(idx1);
          messageText.trim();
          if (smsCallback) {
            //It will run the handleSMS function in the main program
            smsCallback(phoneNumber, messageText);
          }
        }
    }

  
    /*************************************************************************************************
     * function should be declared as "const" since it do not modify any member variables:
	 * check if the SIM800L module is ready by sending "AT" command and waiting for a response.
	 * 
     *************************************************************************************************/
    bool isModuleReady() const {
      int tries = 0;
      while (tries < 20) {
        if (tryATcommand("AT")) {
          return true;
        }
        Serial.print(".");
        tries++;
      }
      return false;
    }
    
    /*************************************************************************************************
     * function should be declared as "const" since it do not modify any member variables:
	 * The purpose of this function is to wait for a response from the SIM800L module and 
	 * check if the response contains the expected answer.
     *************************************************************************************************/
    bool getResponse(const String& expectedAnswer, unsigned int timeout = 1000) const {
      unsigned long startTime = millis();
      //simSerial.read();
      while (millis() - startTime < timeout) {
        //while(simSerial.available()){
        if(simSerial.available()){
          String response = simSerial.readString();
          //FOR DEBUGGING
          //Serial.println(response); 
          if (response.indexOf(expectedAnswer) != -1) {
            Serial.println(response);
            return true;
          }
        }
      }
    
      return false;
    }
    
    /*************************************************************************************************
     * function should be declared as "const" since it do not modify any member variables:
	 * The function sends the AT command to the module using the "println" function of software serial
     *************************************************************************************************/
    bool tryATcommand(const String& cmd, const String& expectedAnswer = "OK", unsigned int timeout=1000) const {
      simSerial.println(cmd);
      return getResponse(expectedAnswer, timeout);
    }
    
    /******************************************************************
     * 
     ******************************************************************/
    String getPhoneNumber() {
      return phoneNumber;
    }
    
    /******************************************************************
     * 
     ******************************************************************/
    String getMessageText() {
      return messageText;
    }

    /******************************************************************
     * 
     ******************************************************************/
    void sendSMS(const String& phoneNumber, const String& messageText) {
      if (tryATcommand("AT+CMGF=1;")) {
        if (tryATcommand("AT+CMGS=\"" + phoneNumber + "\"\r", ">")) {
          simSerial.print(messageText);
          simSerial.write(0x1A); // Send CTRL+Z character to end message
          if (getResponse("+CMGS:", 10000)) {
            Serial.println("Message sent successfully");
            return;
          }
        }
      }
      
      Serial.println("Error: Failed to send message");
      

    }

    /******************************************************************
     * 
     ******************************************************************/
    void setDebug(bool flag) {
      debug = flag;
    }
        
    /******************************************************************
     * //It will called the handleSMS() function in the main program
     ******************************************************************/
    void setSMSCallback(void (*callback)(String, String)) {
      //It is handleSMS() function in the main program
      smsCallback = callback;
    }
  
    void reset() {
      // If a reset pin is defined, use it to reset the module
      if (resetPin != 0) {
        digitalWrite(resetPin, LOW);
        delay(100);
        digitalWrite(resetPin, HIGH);
      }
    
      // Wait for the module to respond
      //return getResponse("SMS Ready", 15000);
    }

	
};
