#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include "TBmanager.h"
#include "ModbusRTUMaster.h"

// --- ค่าเริ่มต้น ---
#define DEFAULT_WIFI_SSID "YourDefaultSSID"
#define DEFAULT_WIFI_PASSWORD "YourDefaultPassword"
#define DEFAULT_TB_SERVER "thingsboard.cloud"
#define DEFAULT_TB_TOKEN "YourDefaultToken"

// สำหรับ Modbus
extern ModbusRTUMaster modbus; // ต้องประกาศ global ใน main
extern uint8_t slaveId;
extern uint8_t slaveId2;
extern void preTransmission();
extern void postTransmission();

class CommandLineManager {
private:
    TBmanager* tbManager;
    Preferences* prefs;

    // State machine
    enum MenuState {
        MAIN_MENU,
        SHOW_STATUS,
        SCAN_WIFI,
        CONNECTION_SETTINGS_MENU,
        SET_SSID,
        SET_PASSWORD,
        SET_SERVER,
        SET_TOKEN,
        SAVE_AND_REBOOT,
        SHOW_NPK,
        SHOW_SENSOR,
        SHOW_SOIL   
        
    };

    MenuState currentState = MAIN_MENU;
    bool menuNeedsDisplay = true;

    // Temp settings
    String tempSsid;
    String tempPass;
    String tempServer;
    String tempToken;

    void displayMainMenu() {
        Serial.println("\n===== Main Menu =====");
        Serial.println("1: Check System Status");
        Serial.println("2: Connection Settings");
        Serial.println("3: Scan for WiFi");
        Serial.println("4: Show NPK values"); // ตัวเลือกใหม่
        Serial.println("5: Show Sensor values"); // ตัวเลือกใหม่
        Serial.println("6: Show Soil Moisture"); 
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
        while (Serial.available() > 0) Serial.read();
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
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        input.trim();
        return input;
    }

    void showStatus() {
        Serial.println("\n--- System Status ---");
        if (tbManager && tbManager->get_wifiStatus()) {
            Serial.print("WiFi Status: Connected to '"); Serial.print(tbManager->get_SSID()); Serial.println("'");
            Serial.print("IP Address: "); Serial.println(tbManager->get_ipAddr());
            if (tbManager->get_thingsboardStatus()) {
                Serial.println("ThingsBoard: Connected");
                if (tbManager->get_rpcStatus()) Serial.println("RPC Service: Active");
                else Serial.println("RPC Service: Not Subscribed");
            } else Serial.println("ThingsBoard: Disconnected");
        } else Serial.println("WiFi Status: Disconnected");
        Serial.println("-----------------------");
        Serial.print("\n>>> Press 0 and Enter to return to Main Menu <<<");
    }

    void scanForWiFi() {
        Serial.println("\nStarting WiFi Scan...");
        int n = WiFi.scanNetworks();
        Serial.println("Scan done.");
        if (n == 0) Serial.println("No networks found.");
        else {
            Serial.print(n); Serial.println(" networks found:");
            Serial.println("----------------------------------------------------");
            for (int i = 0; i < n; ++i) {
                Serial.print(i+1); Serial.print(": "); Serial.print(WiFi.SSID(i));
                Serial.print(" ("); Serial.print(WiFi.RSSI(i)); Serial.print(")");
                Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : " *");
            }
            Serial.println("----------------------------------------------------");
        }
        Serial.print("\n>>> Press 0 and Enter to return to Main Menu <<<");
    }

    void showNPKLoop()
    {
        uint16_t nitrogen = 0, phosphorus = 0, potassium = 0;
        preTransmission();
        ModbusRTUMasterError errN = modbus.readHoldingRegisters(slaveId, 13, &nitrogen, 1);
        ModbusRTUMasterError errP = modbus.readHoldingRegisters(slaveId, 15, &phosphorus, 1);
        ModbusRTUMasterError errK = modbus.readHoldingRegisters(slaveId, 16, &potassium, 1);
        postTransmission();

        if (errN == MODBUS_RTU_MASTER_SUCCESS && errP == MODBUS_RTU_MASTER_SUCCESS && errK == MODBUS_RTU_MASTER_SUCCESS)
        {
            Serial.printf("N:%d P:%d K:%d\n", nitrogen, phosphorus, potassium);
            Serial.println("Press q to main menu\n");
        }
        else
        {
            Serial.printf("Modbus Error N:%d P:%d K:%d\n", errN, errP, errK);
        }
    }

    void showSensorLoop()
    {
        uint16_t rawTemp = 0;
        uint16_t rawLight = 0;
        uint16_t rawHum = 0;

        preTransmission();
        ModbusRTUMasterError errTemp = modbus.readHoldingRegisters(slaveId2, 1, &rawTemp, 1);   // register 1 = temp
        ModbusRTUMasterError errLight = modbus.readHoldingRegisters(slaveId2, 2, &rawLight, 1); // register 2 = light
        ModbusRTUMasterError errHum = modbus.readHoldingRegisters(slaveId2, 0, &rawHum, 1);     // register 3 = humidity
        postTransmission();

        if (errTemp == MODBUS_RTU_MASTER_SUCCESS &&
            errLight == MODBUS_RTU_MASTER_SUCCESS &&
            errHum == MODBUS_RTU_MASTER_SUCCESS)
        {
            // สมมติสูตรการแปลง:
            // อุณหภูมิ: raw / 10 -> °C
            // ความชื้น: raw / 10 -> %
            // แสง: raw * 1 -> lux (ตรงตัว)
            float temperature = rawTemp / 10.0;
            float humidity = rawHum / 10.0;
            float light = rawLight * 1.0; // ปรับตาม datasheet

            Serial.printf("Temp: %.1f C, Humidity: %.1f %%, Light: %.0f lux\n", temperature, humidity, light);
            Serial.println("Press q to main menu\n");
        }
        else
        {
            Serial.printf("Modbus Error Temp:%d Light:%d Humidity:%d\n", errTemp, errLight, errHum);
        }
    }

    void showSoilLoop()
    {
        int rawValue = analogRead(34); // ✅ ใช้ GPIO34 (ADC1_CH6)
        float percent = map(rawValue, 4095, 1400, 0, 100);

        Serial.printf("Soil Moisture Raw: %d, %.1f %%\n", rawValue, percent);
        Serial.println("Press q to main menu\n");
    }
    
    void cliTask() {
        while(true){
            switch(currentState){
                case MAIN_MENU:
                    if(menuNeedsDisplay) displayMainMenu();
                    if(Serial.available()>0){
                        String choice = readSerialInput();
                        if(choice=="1") currentState = SHOW_STATUS;
                        else if(choice=="2") currentState = CONNECTION_SETTINGS_MENU;
                        else if(choice=="3") currentState = SCAN_WIFI;
                        else if(choice=="4") currentState = SHOW_NPK;
                        else if(choice=="5") currentState = SHOW_SENSOR;
                        else if(choice=="6") currentState = SHOW_SOIL;
                        else if(choice.length()>0) Serial.println("Invalid choice");
                        menuNeedsDisplay = true;
                    }
                    break;

                case SHOW_STATUS:
                    showStatus();
                    while(currentState==SHOW_STATUS){
                        String returnChoice = readSerialInput();
                        if(returnChoice=="0"){ currentState=MAIN_MENU; menuNeedsDisplay=true; }
                        else if(returnChoice.length()>0) Serial.print("Press 0 to return: ");
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    break;

                case SCAN_WIFI:
                    scanForWiFi();
                    while(currentState==SCAN_WIFI){
                        String returnChoice = readSerialInput();
                        if(returnChoice=="0"){ currentState=MAIN_MENU; menuNeedsDisplay=true; }
                        else if(returnChoice.length()>0) Serial.print("Press 0 to return: ");
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    break;

                case CONNECTION_SETTINGS_MENU:
                    if(menuNeedsDisplay) displayConnectionSettingsMenu();
                    if(Serial.available()>0){
                        String choice = readSerialInput();
                        if(choice=="1") currentState=SET_SSID;
                        else if(choice=="2") currentState=SET_PASSWORD;
                        else if(choice=="3") currentState=SET_SERVER;
                        else if(choice=="4") currentState=SET_TOKEN;
                        else if(choice=="5") currentState=SAVE_AND_REBOOT;
                        else if(choice=="0") currentState=MAIN_MENU;
                        else if(choice.length()>0) Serial.println("Invalid choice");
                        menuNeedsDisplay=true;
                    }
                    break;

                case SET_SSID:
                    Serial.print("Enter new WiFi SSID: ");
                    tempSsid = readSerialInput();
                    Serial.println("SSID updated.");
                    currentState = CONNECTION_SETTINGS_MENU;
                    menuNeedsDisplay=true;
                    break;

                case SET_PASSWORD:
                    Serial.print("Enter new WiFi Password: ");
                    tempPass = readSerialInput();
                    Serial.println("Password updated.");
                    currentState = CONNECTION_SETTINGS_MENU;
                    menuNeedsDisplay=true;
                    break;

                case SET_SERVER:
                    Serial.print("Enter new ThingsBoard Server: ");
                    tempServer = readSerialInput();
                    Serial.println("Server updated.");
                    currentState = CONNECTION_SETTINGS_MENU;
                    menuNeedsDisplay=true;
                    break;

                case SET_TOKEN:
                    Serial.print("Enter new Device Token: ");
                    tempToken = readSerialInput();
                    Serial.println("Token updated.");
                    currentState = CONNECTION_SETTINGS_MENU;
                    menuNeedsDisplay=true;
                    break;

                case SAVE_AND_REBOOT:
                    Serial.println("\nSaving settings...");
                    prefs->begin("conn-info", false);
                    prefs->putString("ssid", tempSsid);
                    prefs->putString("password", tempPass);
                    prefs->putString("server", tempServer);
                    prefs->putString("token", tempToken);
                    prefs->end();
                    Serial.println("Saved. Rebooting in 3s...");
                    vTaskDelay(pdMS_TO_TICKS(3000));
                    ESP.restart();
                    break;

                case SHOW_NPK:
                    showNPKLoop(); // แสดงค่าตามปกติ
                    // เช็กเฉพาะว่ามี input ใหม่ตอนอยู่ใน state นี้
                    if (Serial.available() > 0)
                    {
                        char cmd = Serial.read();
                        if (cmd == 'q')
                        { // กด q เพื่อออก
                            currentState = MAIN_MENU;
                            menuNeedsDisplay = true;
                        }
                    }

                    vTaskDelay(pdMS_TO_TICKS(1000)); // delay 1 วิ ก่อนอ่านค่าใหม่
                    break;

                case SHOW_SENSOR:
                    showSensorLoop(); // แสดงค่าตามปกติ
                    // เช็กเฉพาะว่ามี input ใหม่ตอนอยู่ใน state นี้
                    if (Serial.available() > 0)
                    {
                        char cmd = Serial.read();
                        if (cmd == 'q')
                        { // กด q เพื่อออก
                            currentState = MAIN_MENU;
                            menuNeedsDisplay = true;
                        }
                    }

                    vTaskDelay(pdMS_TO_TICKS(1000)); // delay 1 วิ ก่อนอ่านค่าใหม่
                    break;

                case SHOW_SOIL:
                    showSoilLoop();
                    if (Serial.available() > 0)
                    {
                        char cmd = Serial.read();
                        if (cmd == 'q')
                        {
                            currentState = MAIN_MENU;
                            menuNeedsDisplay = true;
                        }
                    }
                    vTaskDelay(pdMS_TO_TICKS(1000)); // delay 1 วิ
                    break;
                }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

    static void taskWrapper(void* pvParameters){
        static_cast<CommandLineManager*>(pvParameters)->cliTask();
    }

public:
    CommandLineManager(TBmanager* tb, Preferences* p):tbManager(tb), prefs(p){}
    void loadSettings(){
        prefs->begin("conn-info", true);
        tempSsid = prefs->getString("ssid", DEFAULT_WIFI_SSID);
        tempPass = prefs->getString("password", DEFAULT_WIFI_PASSWORD);
        tempServer = prefs->getString("server", DEFAULT_TB_SERVER);
        tempToken = prefs->getString("token", DEFAULT_TB_TOKEN);
        prefs->end();
    }

    void begin(){
        xTaskCreate(taskWrapper,"CLI_Task", 1024*2,this,1,NULL);
    }

    bool shouldEnterMenuOnBoot(unsigned long waitTime){
        Serial.println("\n[ACTION] Press any key within 10 seconds to enter setup menu.");
        unsigned long startTime=millis();
        while(millis()-startTime<waitTime){
            if(Serial.available()>0){
                while(Serial.available()>0) Serial.read();
                return true;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        return false;
    }

    const char* getSsid() { return tempSsid.c_str(); }
    const char* getPassword() { return tempPass.c_str(); }
    const char* getServer() { return tempServer.c_str(); }
    const char* getToken() { return tempToken.c_str(); }
};

#endif // COMMANDLINE_H
