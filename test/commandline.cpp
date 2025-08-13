#include "commandline.h"
#include <WiFi.h> // ต้อง include สำหรับฟังก์ชันสแกน WiFi

// --- กำหนดค่าเริ่มต้น ---
const char* CommandLine::DEFAULT_WIFI_SSID    = "Pooh";
const char* CommandLine::DEFAULT_WIFI_PASSWORD  = "87654321";
const char* CommandLine::DEFAULT_TB_SERVER    = "thingsboard.cloud";
const char* CommandLine::DEFAULT_TB_TOKEN     = "qvnbqPpZmr5S0w680KPW";

// Constructor: จะถูกเรียกเมื่อ new CommandLine() ทำงาน และจะโหลดค่าให้ทันที
CommandLine::CommandLine() : tbClient(nullptr), currentState(MAIN_MENU), menuNeedsDisplay(true) {
    loadSettings();
}

void CommandLine::setClient(TBmanager* client) {
    this->tbClient = client;
}

// Getters: สำหรับให้โค้ดภายนอกเรียกใช้ค่า
const char* CommandLine::getSsid() const { return tempSsid.c_str(); }
const char* CommandLine::getPassword() const { return tempPass.c_str(); }
const char* CommandLine::getServer() const { return tempServer.c_str(); }
const char* CommandLine::getToken() const { return tempToken.c_str(); }

// Static wrapper สำหรับ FreeRTOS task
void CommandLine::taskWrapper(void* pvParameters) {
    reinterpret_cast<CommandLine*>(pvParameters)->monitorTask();
}

void CommandLine::begin() {
    Serial.println("\n--- ค่าที่ตั้งไว้ปัจจุบัน ---");
    Serial.print("  SSID: "); Serial.println(tempSsid);
    Serial.print("  Server: "); Serial.println(tempServer);
    Serial.print("  Token: "); Serial.println(tempToken);
    Serial.println("-------------------------");

    Serial.println("\n[ACTION] กดปุ่มใดๆ ภายใน 10 วินาที เพื่อเข้าสู่เมนูตั้งค่า");
    Serial.println("         (หากไม่กด จะเริ่มเชื่อมต่ออัตโนมัติ)");
    
    bool enterMenu = false;
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
        if (Serial.available() > 0) {
            enterMenu = true;
            while(Serial.available() > 0) Serial.read(); // Clear buffer
            break;
        }
        delay(50);
    }

    if (enterMenu) {
        Serial.println("\n>> ผู้ใช้ขัดจังหวะ, เข้าสู่เมนูตั้งค่า...");
    } else {
        if (tbClient) {
            Serial.println("\n>> หมดเวลา, กำลังเริ่มการเชื่อมต่อเบื้องหลัง...");
            tbClient->begin(); // เริ่มการเชื่อมต่อจริง
        } else {
            Serial.println("\n[ERROR] TBmanager client is not set. Cannot connect.");
        }
    }

    // สร้าง Task ของเมนูเสมอ
    xTaskCreate(taskWrapper, "Monitor Task", 4096, this, 1, NULL);
}

void CommandLine::loadSettings() {
    preferences.begin("conn-info", true); // เปิดโหมด read-only
    tempSsid   = preferences.getString("ssid", DEFAULT_WIFI_SSID);
    tempPass   = preferences.getString("password", DEFAULT_WIFI_PASSWORD);
    tempServer = preferences.getString("server", DEFAULT_TB_SERVER);
    tempToken  = preferences.getString("token", DEFAULT_TB_TOKEN);
    preferences.end();
}

void CommandLine::displayMainMenu() {
    Serial.println("\n===== Main Menu =====");
    Serial.println("1: Check System Status");
    Serial.println("2: Connection Settings");
    Serial.println("3: Scan for WiFi");
    Serial.print("Please enter your choice and press Enter: ");
    menuNeedsDisplay = false;
}

void CommandLine::displayConnectionSettingsMenu() {
    Serial.println("\n--- Connection Settings ---");
    Serial.print("  1: Set SSID      (current: '"); Serial.print(tempSsid); Serial.println("')");
    Serial.print("  2: Set Password  (current: '"); Serial.print(String(tempPass).length() > 0 ? "********" : "<empty>"); Serial.println("')");
    Serial.print("  3: Set Server    (current: '"); Serial.print(tempServer); Serial.println("')");
    Serial.print("  4: Set Token     (current: '"); Serial.print(tempToken); Serial.println("')");
    Serial.println("---------------------------");
    Serial.println("  5: Save and Reboot");
    Serial.println("  0: Back to Main Menu");
    Serial.print("Please enter your choice: ");
    menuNeedsDisplay = false;
}

String CommandLine::readSerialInput() {
    while (Serial.available() > 0) { Serial.read(); }
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

void CommandLine::monitorTask() {
    while (true) {
        handleMenu();
        delay(50);
    }
}

void CommandLine::handleMenu() {
    switch (currentState) {
        case MAIN_MENU:
            if (menuNeedsDisplay) displayMainMenu();
            if (Serial.available() > 0) {
                String choice = readSerialInput();
                if (choice == "1") currentState = SHOW_STATUS;
                else if (choice == "2") currentState = CONNECTION_SETTINGS_MENU;
                else if (choice == "3") currentState = SCAN_WIFI;
                else if (choice.length() > 0) Serial.println("Invalid choice, please try again.");
                menuNeedsDisplay = true;
            }
            break;

        case SHOW_STATUS: {
            Serial.println("\n--- System Status ---");
            if (tbClient && tbClient->get_wifiStatus()) {
                Serial.print("WiFi Status: Connected to '"); Serial.print(tbClient->get_SSID()); Serial.println("'");
                Serial.print("IP Address: "); Serial.println(tbClient->get_ipAddr());
                if (tbClient->get_thingsboardStatus()) {
                    Serial.println("ThingsBoard: Connected");
                    if (tbClient->get_rpcStatus()) Serial.println("RPC Service: Subscribed & Active");
                    else Serial.println("RPC Service: Not Subscribed (Connecting...)");
                } else {
                    Serial.println("ThingsBoard: Disconnected (Connecting...)");
                }
            } else {
                Serial.println("WiFi Status: Disconnected");
            }
            Serial.println("-----------------------");
            Serial.print("\n>>> Press 0 and Enter to return to Main Menu <<<");
            while (currentState == SHOW_STATUS) {
                if (readSerialInput() == "0") {
                    currentState = MAIN_MENU;
                    menuNeedsDisplay = true;
                    break;
                } else {
                    Serial.print("\nInvalid choice. Press 0 and Enter to return: ");
                }
            }
            break;
        }

        case SCAN_WIFI: {
            Serial.println("\nStarting WiFi Scan...");
            int n = WiFi.scanNetworks();
            Serial.println("Scan done.");
            if (n == 0) {
                Serial.println("No networks found.");
            } else {
                Serial.printf("%d networks found:\n", n);
                Serial.println("----------------------------------------------------");
                for (int i = 0; i < n; ++i) {
                    Serial.printf("%d: %s (%d)%s\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : " *");
                }
                Serial.println("----------------------------------------------------");
            }
            Serial.print("\n>>> Press 0 and Enter to return to Main Menu <<<");
            while (currentState == SCAN_WIFI) {
                if (readSerialInput() == "0") {
                    currentState = MAIN_MENU;
                    menuNeedsDisplay = true;
                    break;
                } else {
                    Serial.print("\nInvalid choice. Press 0 and Enter to return: ");
                }
            }
            break;
        }
        
        case CONNECTION_SETTINGS_MENU: {
            if (menuNeedsDisplay) displayConnectionSettingsMenu();
            if (Serial.available() > 0) {
                String choice = readSerialInput();
                if (choice == "1") currentState = SET_SSID;
                else if (choice == "2") currentState = SET_PASSWORD;
                else if (choice == "3") currentState = SET_SERVER;
                else if (choice == "4") currentState = SET_TOKEN;
                else if (choice == "5") currentState = SAVE_AND_REBOOT;
                else if (choice == "0") currentState = MAIN_MENU;
                else Serial.println("Invalid choice, please try again.");
                menuNeedsDisplay = true;
            }
            break;
        }

        case SET_SSID:
            Serial.print("Enter new WiFi SSID and press Enter: ");
            tempSsid = readSerialInput();
            Serial.println("SSID updated.");
            currentState = CONNECTION_SETTINGS_MENU;
            menuNeedsDisplay = true;
            break;
        
        case SET_PASSWORD:
            Serial.print("Enter new WiFi Password and press Enter: ");
            tempPass = readSerialInput();
            Serial.println("Password updated.");
            currentState = CONNECTION_SETTINGS_MENU;
            menuNeedsDisplay = true;
            break;

        case SET_SERVER:
            Serial.print("Enter new ThingsBoard Server and press Enter: ");
            tempServer = readSerialInput();
            Serial.println("Server updated.");
            currentState = CONNECTION_SETTINGS_MENU;
            menuNeedsDisplay = true;
            break;

        case SET_TOKEN:
            Serial.print("Enter new Device Token and press Enter: ");
            tempToken = readSerialInput();
            Serial.println("Token updated.");
            currentState = CONNECTION_SETTINGS_MENU;
            menuNeedsDisplay = true;
            break;

        case SAVE_AND_REBOOT:
            Serial.println("\nSaving new connection settings...");
            preferences.begin("conn-info", false); // เปิดโหมด read-write
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
}