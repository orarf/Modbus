#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <Arduino.h>
#include <Preferences.h>
#include "TBmanager.h"

// --- ค่าเริ่มต้น (Default Values) ---
#define DEFAULT_WIFI_SSID "YourDefaultSSID"
#define DEFAULT_WIFI_PASSWORD "YourDefaultPassword"
#define DEFAULT_TB_SERVER "thingsboard.cloud"
#define DEFAULT_TB_TOKEN "YourDefaultToken"

class CommandLineManager {
private:
    TBmanager* tbManager;
    Preferences* prefs;

    // State machine for the menu
    enum MenuState {
        MAIN_MENU,
        SHOW_STATUS,
        SCAN_WIFI,
        CONNECTION_SETTINGS_MENU,
        SET_SSID,
        SET_PASSWORD,
        SET_SERVER,
        SET_TOKEN,
        SAVE_AND_REBOOT
    };

    MenuState currentState = MAIN_MENU;
    bool menuNeedsDisplay = true;

    // Temporary storage for settings being edited
    String tempSsid;
    String tempPass;
    String tempServer;
    String tempToken;

    // Private methods for internal logic
    void displayMainMenu() {
        Serial.println("\n===== Main Menu =====");
        Serial.println("1: Check System Status");
        Serial.println("2: Connection Settings");
        Serial.println("3: Scan for WiFi");
        Serial.print("Please enter your choice and press Enter: ");
        menuNeedsDisplay = false;
    }

    void displayConnectionSettingsMenu() {
        Serial.println("\n--- Connection Settings ---");
        Serial.print("  1: Set SSID     (current: '"); Serial.print(tempSsid); Serial.println("')");
        Serial.print("  2: Set Password   (current: '"); Serial.print(String(tempPass).length() > 0 ? "********" : "<empty>"); Serial.println("')");
        Serial.print("  3: Set Server     (current: '"); Serial.print(tempServer); Serial.println("')");
        Serial.print("  4: Set Token      (current: '"); Serial.print(tempToken); Serial.println("')");
        Serial.println("---------------------------");
        Serial.println("  5: Save and Reboot");
        Serial.println("  0: Back to Main Menu");
        Serial.print("Please enter your choice: ");
        menuNeedsDisplay = false;
    }

    String readSerialInput() {
        // Clear any previous leftover characters in the buffer
        while (Serial.available() > 0) {
            Serial.read();
        }
        
        String input = "";
        while (true) {
            if (Serial.available()) {
                char c = Serial.read();
                if (c == '\r' || c == '\n') { // Enter key pressed
                    Serial.println();
                    break;
                } else if (c == 127 || c == 8) { // Backspace
                    if (input.length() > 0) {
                        input.remove(input.length() - 1);
                        Serial.print("\b \b"); // Erase character on terminal
                    }
                } else {
                    input += c;
                    Serial.print(c);
                }
            }
            // Add a small delay to prevent the loop from running too fast
            vTaskDelay(pdMS_TO_TICKS(20)); 
        }
        input.trim();
        return input;
    }

    // The main task loop for the CLI
    void cliTask() {
        while (true) {
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

                case SHOW_STATUS:
                    showStatus(); // Call dedicated function
                    // Wait for user to return to main menu
                    while (currentState == SHOW_STATUS) {
                        String returnChoice = readSerialInput();
                        if (returnChoice == "0") {
                            currentState = MAIN_MENU;
                            menuNeedsDisplay = true;
                        } else if (returnChoice.length() > 0) {
                            Serial.print("\nInvalid choice. Press 0 and Enter to return: ");
                        }
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    break;

                case SCAN_WIFI:
                    scanForWiFi(); // Call dedicated function
                    // Wait for user to return to main menu
                    while (currentState == SCAN_WIFI) {
                        String returnChoice = readSerialInput();
                        if (returnChoice == "0") {
                            currentState = MAIN_MENU;
                            menuNeedsDisplay = true;
                        } else if (returnChoice.length() > 0) {
                           Serial.print("\nInvalid choice. Press 0 and Enter to return: ");
                        }
                         vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    break;

                case CONNECTION_SETTINGS_MENU:
                    if (menuNeedsDisplay) displayConnectionSettingsMenu();
                    if (Serial.available() > 0) {
                        String choice = readSerialInput();
                        if (choice == "1") currentState = SET_SSID;
                        else if (choice == "2") currentState = SET_PASSWORD;
                        else if (choice == "3") currentState = SET_SERVER;
                        else if (choice == "4") currentState = SET_TOKEN;
                        else if (choice == "5") currentState = SAVE_AND_REBOOT;
                        else if (choice == "0") currentState = MAIN_MENU;
                        else if (choice.length() > 0) Serial.println("Invalid choice, please try again.");
                        menuNeedsDisplay = true;
                    }
                    break;

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
                    prefs->begin("conn-info", false); // Open in read-write mode
                    prefs->putString("ssid", tempSsid);
                    prefs->putString("password", tempPass);
                    prefs->putString("server", tempServer);
                    prefs->putString("token", tempToken);
                    prefs->end();
                    Serial.println("Settings saved!");
                    Serial.println("Restarting in 3 seconds to apply changes...");
                    vTaskDelay(pdMS_TO_TICKS(3000));
                    ESP.restart();
                    break;
            }
            vTaskDelay(pdMS_TO_TICKS(50)); // Main task delay
        }
    }

    void showStatus() {
        Serial.println("\n--- System Status ---");
        if (tbManager->get_wifiStatus()) {
            Serial.print("WiFi Status: Connected to '");
            Serial.print(tbManager->get_SSID());
            Serial.println("'");
            Serial.print("IP Address: ");
            Serial.println(tbManager->get_ipAddr());
            if (tbManager->get_thingsboardStatus()) {
                Serial.println("ThingsBoard: Connected");
                if (tbManager->get_rpcStatus()) {
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
    }

    void scanForWiFi() {
        Serial.println("\nStarting WiFi Scan...");
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
    }


    // Static wrapper to launch the task, as xTaskCreate needs a static function
    static void taskWrapper(void* pvParameters) {
        static_cast<CommandLineManager*>(pvParameters)->cliTask();
    }

public:
    // Constructor
    CommandLineManager(TBmanager* tb, Preferences* p) : tbManager(tb), prefs(p) {}

    // Method to load initial settings from Preferences
    void loadSettings() {
        prefs->begin("conn-info", true); // Open in read-only mode
        tempSsid   = prefs->getString("ssid", DEFAULT_WIFI_SSID);
        tempPass   = prefs->getString("password", DEFAULT_WIFI_PASSWORD);
        tempServer = prefs->getString("server", DEFAULT_TB_SERVER);
        tempToken  = prefs->getString("token", DEFAULT_TB_TOKEN);
        prefs->end();
    }

    // Method to start the CLI task
    void begin() {
        xTaskCreate(
            taskWrapper,      // Function to implement the task
            "CLI_Task",       // Name of the task
            1024*2,             // Stack size in words
            this,             // Task input parameter (pointer to this instance)
            1,                // Priority of the task
            NULL              // Task handle
        );
    }

    // Checks if user wants to enter menu on startup
    bool shouldEnterMenuOnBoot(unsigned long waitTime) {
        Serial.println("\n[ACTION] Press any key within 10 seconds to enter setup menu.");
        Serial.println("         (Otherwise, will connect automatically in the background)");
        
        unsigned long startTime = millis();
        while (millis() - startTime < waitTime) {
            if (Serial.available() > 0) {
                while(Serial.available() > 0) Serial.read(); // Clear buffer
                return true;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        return false;
    }

    // Getters for the loaded settings so main can use them to init TBmanager
    const char* getSsid() { return tempSsid.c_str(); }
    const char* getPassword() { return tempPass.c_str(); }
    const char* getServer() { return tempServer.c_str(); }
    const char* getToken() { return tempToken.c_str(); }
};

#endif // COMMANDLINE_H
