#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <NewPing.h>
#include <SoftwareSerial.h>
#include <toneAC.h>

// Ultrasonic sensor pins and parameters
#define TRIGGER_PIN 6
#define ECHO_PIN 5
#define MAX_DISTANCE 400

// RFID pins
#define RST_PIN 9
#define SS_PIN 10

// Buzzer pin
#define BUZZER_PIN 7

// Bluetooth pins
#define BLUETOOTH_TX 2
#define BLUETOOTH_RX 3
SoftwareSerial bluetooth(BLUETOOTH_TX, BLUETOOTH_RX);

// Initialize NewPing for ultrasonic sensor and MFRC522 for RFID
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
MFRC522 rfid(SS_PIN, RST_PIN);

// Constants for height calculations and access control
const int SENSOR_HEIGHT = 208;
const int MIN_HEIGHT_FOR_ACCESS = 100;

// Structure to hold access control results
struct AccessResult {
  bool accessGranted;
  String userName;
};

// Structure for authorized users
struct User {
  String name;
  String uid;
};

// List of authorized users with their names and UID
User authorizedUsers[] = {
  { "Abdalla Nassar", "4 4a f5 6a 2c 59 80" },
  { "Nafea", "fe 8a 2 bf" },
  { "samy", "9F 30 19 89" }
};

const int numUsers = sizeof(authorizedUsers) / sizeof(User);
int peopleCount = 0;

// Setup function
void setup() {
  Serial.begin(9600);     // Initialize serial communication
  bluetooth.begin(9600);  // Initialize Bluetooth communication

  pinMode(BUZZER_PIN, OUTPUT);    // Set BUZZER_PIN as output
  digitalWrite(BUZZER_PIN, LOW);  // Turn off buzzer initially

  SPI.begin();      // Initiate SPI bus
  rfid.PCD_Init();  // Initiate MFRC522 RFID module
}

// Function to sound the buzzer for successful access
void soundBuzzerSuccess() {
  digitalWrite(BUZZER_PIN, HIGH);  // Turn on buzzer
  delay(600);                      // Wait for 500ms
  digitalWrite(BUZZER_PIN, LOW);   // Turn off buzzer
}

// Function to sound the buzzer for denied access
void soundBuzzerFail() {
  toneAC(BUZZER_PIN, 1000, 200);  // Emit a tone for 200ms on BUZZER_PIN
  delay(200);                     // Wait for 200ms before playing another tone
}

// Main loop
void loop() {
  bool accessGranted = false;

  float distance = sonar.ping_cm();  // Measure distance from ultrasonic sensor

  float height = SENSOR_HEIGHT - distance;  // Calculate height based on sensor readings
  Serial.println(height);                   // Print height to serial monitor
  Serial.println(distance);                 // Print distance to serial monitor

  // Check if height measurement is within valid range
  if (height > 0 && height < 400) {
    // Check if height meets access criteria
    if (height < MIN_HEIGHT_FOR_ACCESS) {
      Serial.println("Entry denied: Person is too short.");     // Print message to serial monitor
      bluetooth.println("Entry denied: Person is too short.");  // Send message via Bluetooth
      soundBuzzerFail();                                        // Sound buzzer for denied access
    } else if (height > MIN_HEIGHT_FOR_ACCESS) {
      Serial.println("Please present NFC card for entry.");     // Prompt to present NFC card
      bluetooth.println("Please present NFC card for entry.");  // Send message via Bluetooth
      unsigned long startTime = millis();                       // Record start time for timeout

      // Loop for NFC card detection and verification
      while (millis() - startTime < 8000) {
        if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
          String uidString = "";

          // Construct UID string from RFID card
          for (byte i = 0; i < rfid.uid.size; i++) {
            uidString += String(rfid.uid.uidByte[i], HEX);
            if (i < rfid.uid.size - 1) {
              uidString += " ";
            }
          }

          Serial.println(uidString);  // Print UID to serial monitor
          AccessResult result;
          result.accessGranted = false;

          // Check if UID matches any authorized user
          for (int i = 0; i < numUsers; i++) {
            if (uidString.equals(authorizedUsers[i].uid)) {
              result.accessGranted = true;
              result.userName = authorizedUsers[i].name;
              break;
            }
          }

          // Process access result
          if (result.accessGranted) {
            String userName = result.userName;
            peopleCount++;                          // Increment people count
            Serial.println("Welcome " + userName);  // Print welcome message to serial monitor
            Serial.print("Count of people: ");
            Serial.println(peopleCount);               // Print current people count to serial monitor
            bluetooth.println("Welcome " + userName);  // Send welcome message via Bluetooth
            bluetooth.print("Count of people: ");
            bluetooth.println(peopleCount);  // Send current people count via Bluetooth
            soundBuzzerSuccess();            // Sound buzzer for successful access
            delay(3000);                     // Delay to prevent multiple reads
            break;                           // Exit NFC card detection loop
          } else if (!result.accessGranted) {
            Serial.println("Access denied");     // Print access denied message to serial monitor
            bluetooth.println("Access denied");  // Send access denied message via Bluetooth
            soundBuzzerFail();                   // Sound buzzer for denied access
            delay(1000);                         // Delay to prevent rapid retries
            break;                               // Exit NFC card detection loop
          }
        }
      }
    }
  } else {
    Serial.println("No one under the sensor.");  // Print message for no person detected
    soundBuzzerFail();                           // Sound buzzer for no person detected
    delay(1500);                                 // Delay before next sensor check
  }

  rfid.PICC_HaltA();  // Halt RFID card
}
