// This sketch demonstrates connecting and receiving RPC calls from

// ThingsBoard using ThingsBoard SDK.

//

// Hardware:

//  - Arduino Uno

//  - ESP8266 connected to Arduino Uno

#include <WiFi.h>

#include <TBmanager.h>

#include <Preferences.h>



Preferences preferences;



// --- ค่าเริ่มต้น (Default Values) ---

// จะถูกใช้เมื่อยังไม่มีการตั้งค่าบันทึกไว้ในหน่วยความจำ

#define DEFAULT_WIFI_SSID "Pooh"

#define DEFAULT_WIFI_PASSWORD "87654321"

#define DEFAULT_TB_SERVER "thingsboard.cloud"

#define DEFAULT_TB_TOKEN "qvnbqPpZmr5S0w680KPW" // <<< แก้ไข Token เริ่มต้นตรงนี้



TBmanager *tbClient;



// --- ตัวแปรสำหรับเก็บค่าที่ตั้งในเมนูย่อยชั่วคราว ---

String tempSsid;

String tempPass;

String tempServer;

String tempToken;





void RPC_TEST_process1(const JsonVariantConst &data, JsonDocument &response) {

  Serial.println("Received the json RPC method RPC_TEST_process1");



  // Size of the response document needs to be configured to the size of the innerDoc + 1.

  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;

  innerDoc["string"] = "exampleResponseString";

  innerDoc["int"] = 5;

  innerDoc["float"] = 5.0f;

  innerDoc["bool"] = true;

  response["json_data"] = innerDoc;

}



void RPC_TEST_process2(const JsonVariantConst &data, JsonDocument &response) {

  Serial.println("Received the json RPC method RPC_TEST_process2");



  // Size of the response document needs to be configured to the size of the innerDoc + 1.

  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;

  innerDoc["string"] = "exampleResponseString RPC_TEST_process2";

  innerDoc["int"] = 5;

  innerDoc["float"] = 5.0f;

  innerDoc["bool"] = true;

  response["json_data"] = innerDoc;

}



void RPC_TEST_process3(const JsonVariantConst &data, JsonDocument &response) {

  Serial.println("Received the json RPC method RPC_TEST_process3 ");



  // Size of the response document needs to be configured to the size of the innerDoc + 1.

  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;

  innerDoc["string"] = "exampleResponseString";

  innerDoc["int"] = 5;

  innerDoc["float"] = 5.0f;

  innerDoc["bool"] = true;

  response["json_data"] = innerDoc;

}



    const RPC_Callback xcallbacks[3] = {

      // Requires additional memory in the JsonDocument for the JsonDocument that will be copied into the response

      { "USER_CONTROL_01",    RPC_TEST_process1 },

      // Requires additional memory in the JsonDocument for 5 key-value pairs that do not copy their value into the JsonDocument itself

      { "USER_CONTROL_02",    RPC_TEST_process2 },

      // Internal size can be 0, because if we use the JsonDocument as a JsonVariant and then set the value we do not require additional memory

      { "USER_CONTROL_03",     RPC_TEST_process3 }

    };



// ++ เพิ่ม State สำหรับเมนูย่อยการตั้งค่า ++

enum MenuState {

  MAIN_MENU,

  SHOW_STATUS,

  SCAN_WIFI,

  CONNECTION_SETTINGS_MENU, // State สำหรับแสดงเมนูย่อย

  SET_SSID,                 // State สำหรับตั้งค่า SSID

  SET_PASSWORD,             // State สำหรับตั้งค่า Password

  SET_SERVER,               // State สำหรับตั้งค่า Server

  SET_TOKEN,                // State สำหรับตั้งค่า Token

  SAVE_AND_REBOOT           // State สำหรับบันทึกและรีบูต

};

MenuState currentState = MAIN_MENU;

bool menuNeedsDisplay = true; // ตัวแปรเพื่อช่วยให้เมนูแสดงผลแค่ครั้งเดียว



void displayMainMenu() {

  Serial.println("\n===== Main Menu =====");

  Serial.println("1: Check System Status");

  Serial.println("2: Connection Settings"); // ++ เปลี่ยนชื่อเมนู 2 ++

  Serial.println("3: Scan for WiFi");

  Serial.print("Please enter your choice and press Enter: ");

  menuNeedsDisplay = false;

}



// ++ ฟังก์ชันใหม่สำหรับแสดงเมนูย่อยการตั้งค่า ++

void displayConnectionSettingsMenu() {

    Serial.println("\n--- Connection Settings ---");

    Serial.print("  1: Set SSID      (current: '"); Serial.print(tempSsid); Serial.println("')");

    Serial.print("  2: Set Password  (current: '"); Serial.print(String(tempPass).length() > 0 ? "********" : "<empty>"); Serial.println("')");

    Serial.print("  3: Set Server    (current: '"); Serial.print(tempServer); Serial.println("')");

    Serial.print("  4: Set Token     (current: '"); Serial.print(tempToken); Serial.println("')");

    Serial.println("---------------------------");

    Serial.println("  5: Save and Reboot");

    Serial.println("  0: Back to Main Menu");

    Serial.print("Please enter your choice: ");

    menuNeedsDisplay = false;

}





String readSerialInput() {

  while (Serial.available() > 0) {

    Serial.read();

  }

 

  String input = "";

  while (true) {

    if (Serial.available()) {

      char c = Serial.read();

      if (c == '\r' || c == '\n') {

        Serial.println();

        break;

      } else if (c == 127 || c == 8) {

        if (input.length() > 0) {

          input.remove(input.length() - 1);

          Serial.print("\b \b");

        }

      } else {

        input += c;

        Serial.print(c);

      }

    }

    delay(50);

  }

  input.trim();

  return input;

}



void printScannedNetwork(bool networkFound, int index, int total, String ssid, int32_t rssi, bool isEncrypted) {

    if (index == 1) {

        Serial.print(total);

        Serial.println(" networks found:");

        Serial.println("----------------------------------------------------");

    }



    if (networkFound) {

        Serial.print(index);

        Serial.print(": ");

        Serial.print(ssid);

        Serial.print(" (");

        Serial.print(rssi);

        Serial.print(")");

        Serial.println(isEncrypted ? " *" : " ");

    }

}



void monitorTask(void *pvParameters) {

  while (true) {

    switch (currentState) {

     

      case MAIN_MENU:

        if (menuNeedsDisplay) {

          displayMainMenu();

        }



        if (Serial.available() > 0) {

          String choice = readSerialInput();



          if (choice == "1") {

            currentState = SHOW_STATUS;

          } else if (choice == "2") {

            currentState = CONNECTION_SETTINGS_MENU; // ++ ไปยังเมนูย่อย ++

          } else if (choice == "3") {

            currentState = SCAN_WIFI;

          } else {

            if (choice.length() > 0) {

                 Serial.println("Invalid choice, please try again.");

            }

          }

          menuNeedsDisplay = true;

        }

        break;



      case SHOW_STATUS:{

        Serial.println("\n--- System Status ---");

        if (tbClient->get_wifiStatus()) {

            Serial.print("WiFi Status: Connected to '");

            Serial.print(tbClient->get_SSID());

            Serial.println("'");

            Serial.print("IP Address: ");

            Serial.println(tbClient->get_ipAddr());

            if (tbClient->get_thingsboardStatus()) {

                Serial.println("ThingsBoard: Connected");

                if (tbClient->get_rpcStatus()) {

                    Serial.println("RPC Service: Subscribed & Active");

                } else {

                    Serial.println("RPC Service: Not Subscribed (Connecting...)");

                }

            } else {

                Serial.println("ThingsBoard: Disconnected (Connecting...)");

            }

        } else {

            Serial.println("WiFi Status: Disconnected");

        }

        Serial.println("-----------------------");

        Serial.print("\n>>> Press 0 and Enter to return to Main Menu <<<");



        while (currentState == SHOW_STATUS) {

            String returnChoice = readSerialInput();

            if (returnChoice == "0") {

                currentState = MAIN_MENU;

                menuNeedsDisplay = true;

                break;

            } else {

                Serial.print("\nInvalid choice. Press 0 and Enter to return: ");

            }

        }

        break;

      }

      case SCAN_WIFI:{

        Serial.println("\nStarting WiFi Scan (using standard function)...");

       

        int n = WiFi.scanNetworks();



        Serial.println("Scan done.");

        if (n == 0) {

            Serial.println("No networks found.");

        } else {

            Serial.print(n);

            Serial.println(" networks found:");

            Serial.println("----------------------------------------------------");

            for (int i = 0; i < n; ++i) {

                Serial.print(i + 1);

                Serial.print(": ");

                Serial.print(WiFi.SSID(i));

                Serial.print(" (");

                Serial.print(WiFi.RSSI(i));

                Serial.print(")");

                Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : " *");

            }

            Serial.println("----------------------------------------------------");

        }



        Serial.print("\n>>> Press 0 and Enter to return to Main Menu <<<");



        while (currentState == SCAN_WIFI) {

          String returnChoice = readSerialInput();

          if (returnChoice == "0") {

              currentState = MAIN_MENU;

              menuNeedsDisplay = true;

              break;

          } else {

              Serial.print("\nInvalid choice. Press 0 and Enter to return: ");

          }

        }

        break;

    }

    // +++++ บล็อกโค้ดใหม่ทั้งหมดสำหรับจัดการเมนูย่อย +++++

    case CONNECTION_SETTINGS_MENU: {

        if (menuNeedsDisplay) {

            displayConnectionSettingsMenu();

        }

        if (Serial.available() > 0) {

            String choice = readSerialInput();

            if (choice == "1") currentState = SET_SSID;

            else if (choice == "2") currentState = SET_PASSWORD;

            else if (choice == "3") currentState = SET_SERVER;

            else if (choice == "4") currentState = SET_TOKEN;

            else if (choice == "5") currentState = SAVE_AND_REBOOT;

            else if (choice == "0") currentState = MAIN_MENU;

            else Serial.println("Invalid choice, please try again.");

            menuNeedsDisplay = true; // ตั้งค่าให้แสดงเมนูของ State ถัดไป

        }

        break;

    }

    case SET_SSID: {

        Serial.print("Enter new WiFi SSID and press Enter: ");

        tempSsid = readSerialInput();

        Serial.println("SSID updated.");

        currentState = CONNECTION_SETTINGS_MENU;

        menuNeedsDisplay = true;

        break;

    }

    case SET_PASSWORD: {

        Serial.print("Enter new WiFi Password and press Enter: ");

        tempPass = readSerialInput();

        Serial.println("Password updated.");

        currentState = CONNECTION_SETTINGS_MENU;

        menuNeedsDisplay = true;

        break;

    }

    case SET_SERVER: {

        Serial.print("Enter new ThingsBoard Server and press Enter: ");

        tempServer = readSerialInput();

        Serial.println("Server updated.");

        currentState = CONNECTION_SETTINGS_MENU;

        menuNeedsDisplay = true;

        break;

    }

    case SET_TOKEN: {

        Serial.print("Enter new Device Token and press Enter: ");

        tempToken = readSerialInput();

        Serial.println("Token updated.");

        currentState = CONNECTION_SETTINGS_MENU;

        menuNeedsDisplay = true;

        break;

    }

    case SAVE_AND_REBOOT: {

        Serial.println("\nSaving new connection settings...");

       

        preferences.begin("conn-info", false); // เปิด Preferences ในโหมด read-write

        preferences.putString("ssid", tempSsid);

        preferences.putString("password", tempPass);

        preferences.putString("server", tempServer);

        preferences.putString("token", tempToken);

        preferences.end();



        Serial.println("Settings saved!");

        Serial.println("Restarting in 3 seconds to apply changes...");

        delay(3000);

        ESP.restart();

        break;

    }

    // +++++ สิ้นสุดบล็อกโค้ดใหม่ +++++

  }

    delay(50);

  }

}





void setup() {

    delay(100);

    Serial.begin(9600);

    Serial.println("\n\n===== เริ่มการทำงาน =====");



    // --- โหลดค่าที่บันทึกไว้ ---

    preferences.begin("conn-info", true);

    tempSsid   = preferences.getString("ssid", DEFAULT_WIFI_SSID);

    tempPass   = preferences.getString("password", DEFAULT_WIFI_PASSWORD);

    tempServer = preferences.getString("server", DEFAULT_TB_SERVER);

    tempToken  = preferences.getString("token", DEFAULT_TB_TOKEN);

    preferences.end();

 

    Serial.println("--- โหลดค่าที่ตั้งไว้เรียบร้อย ---");

     Serial.print("  SSID: "); Serial.println(tempSsid);

    Serial.print("  Server: "); Serial.println(tempServer);

    Serial.print("  Token: "); Serial.println(tempToken);

    Serial.println("-------------------------");

    // สร้าง Instance ของ tbClient

    tbClient = new TBmanager(tempSsid.c_str(), tempPass.c_str(), tempServer.c_str(), tempToken.c_str());

    tbClient->RPCRoute(xcallbacks);



    // --- ให้โอกาสผู้ใช้ขัดจังหวะเพื่อเข้าเมนู ---

    Serial.println("\n[ACTION] กดปุ่มใดๆ ภายใน 10 วินาที เพื่อเข้าสู่เมนู");

    Serial.println("         (หากไม่กด จะเริ่มเชื่อมต่อเบื้องหลังอัตโนมัติ)");

   

    bool enterMenu = false;

    unsigned long startTime = millis();

    while (millis() - startTime < 10000) {

        if (Serial.available() > 0) {

            enterMenu = true;

            while(Serial.available() > 0) Serial.read(); // ล้าง Buffer

            break;

        }

    }



    if (enterMenu) {

        Serial.println("\n>> ผู้ใช้ขัดจังหวะ, เข้าสู่เมนูตั้งค่า");

    } else {

        Serial.println("\n>> หมดเวลา, กำลังเริ่มการเชื่อมต่อเบื้องหลัง...");

        tbClient->begin(); // เริ่มการเชื่อมต่อจริงใน Background

    }



    // เริ่ม Task ของเมนูเสมอ

    xTaskCreate(monitorTask, "Monitor Task", 4096, NULL, 1, NULL);

}

void loop(){

 

}