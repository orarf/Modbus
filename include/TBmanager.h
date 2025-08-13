  // This sketch demonstrates connecting and receiving RPC calls from 
  // ThingsBoard using ThingsBoard SDK.
  //
  // Hardware:
  //  - Arduino Uno
  //  - ESP32S3 connected to Arduino Uno
  #ifndef TBMANAGER_H
  #define TBMANAGER_H
  #define DEBUG
  #include <WiFi.h>
  #include <ThingsBoard.h>  //ThingsBoard@^0.15.0
  #include <Server_Side_RPC.h> //build-in ThingsBoard@^0.15.0
  #include <Arduino_MQTT_Client.h> //build-in ThingsBoard@^0.15.0



  // MQTT port used to communicate with the server, 1883 is the default unencrypted MQTT port,
  // whereas ESP32S3 would be the default encrypted SSL MQTT port
  // constexpr uint16_t THINGSBOARD_PORT = 1883U;
  // Maximum size packets will ever be sent or received by the underlying MQTT client,
  // if the size is to small messages might not be sent or received messages will be discarded
  constexpr uint16_t MAX_MESSAGE_SEND_SIZE = 128U;
  constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 128U;
  // Baud rate for the debugging serial connection
  // If the Serial output is mangled, ensure to change the monitor speed accordingly to this variable
  constexpr char CONNECTING_MSG[] = "Connecting to: (%s) with token (%s)";
  constexpr uint8_t MAX_RPC_SUBSCRIPTIONS = 3U;
  constexpr uint8_t MAX_RPC_RESPONSE = 5U;

  class TBmanager{
    private:
          const char* ssid;
          const char* password;
          const char* tbServer;
          const uint16_t tbPort;
          const char* token;
          RPC_Callback callbacks[MAX_RPC_SUBSCRIPTIONS];
          WiFiClient espClient;
          Arduino_MQTT_Client mqttClient;
          Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;
          IAPI_Implementation* apis[1U] = {&rpc};
          ThingsBoard TBClient;
          bool WIFI_connected = false;
          bool TB_connected = false;
          bool RPC_subscribed = false;
          String localIP;
          String SSID;
    void connectToWiFi() {
      // Attempting to establish a connection to the given WiFi network
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        // Delay 1000ms until a connection has been successfully established
        vTaskDelay(pdMS_TO_TICKS(1000));
        WIFI_connected = false;
      }
      localIP = WiFi.localIP().toString();
      SSID = WiFi.SSID();
      WIFI_connected = true;
    }
    void connectToThingsBoard() {
      if (!TBClient.connected()) 
      {
        TBClient.disconnect();
        rpc.RPC_Unsubscribe();
        TB_connected =  false;
        RPC_subscribed = false;
        if (!TBClient.connect(tbServer, token ,tbPort)) {
          return;
        }else{
          TB_connected = true;
          if (!RPC_subscribed) {
          if (!rpc.RPC_Subscribe(callbacks + 0U, callbacks + 3)) {
            return;
          }else {
            RPC_subscribed = true;
            }   
          }
        }
      }
    }

    void maintainConnection() {
        if (WiFi.status() != WL_CONNECTED) {
            
        connectToWiFi();
        } 
        connectToThingsBoard();
      }

    static void TaskWrapper_TBmanager(void *pvParameters) {
          static_cast<TBmanager*>(pvParameters)->TBmanagerTask(pvParameters);
        }
    void createTxTask() {
                if (xTaskCreatePinnedToCore(TaskWrapper_TBmanager, "TBmanagerTask", Default_Max_Stack_Size*4 , this, 5, NULL, 0) != pdPASS) {
                    #ifdef DEBUG
                    printf("Failed to create Task_TBmanager\n");
                    #endif
                }
            }

    void TBmanagerTask(void *pvParameters) {
          vTaskDelay(pdMS_TO_TICKS(1000));
          connectToWiFi();
          connectToThingsBoard();
      while (true) {
            maintainConnection();
            TBClient.loop();
            vTaskDelay(pdMS_TO_TICKS(500));
      }
    }

    public:
    TBmanager(const char* ssid, const char* password, const char* tbServer, const char* token, const uint16_t tbPort = DEFAULT_MQTT_PORT):
    ssid(ssid), 
    password(password),
    tbServer(tbServer),
    tbPort(tbPort), 
    token(token),
    mqttClient(espClient),
    TBClient(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis + 0U, apis + 1U)
    {}
     void scanWiFi(std::function<void(bool, int, int, String, int32_t, bool)> onNetworkScanned) {
        int n = WiFi.scanNetworks();

        if (n == 0) {
            onNetworkScanned(false, 0, 0, "", 0, false);
        } else {
            // ถ้าเจอเครือข่าย ให้วนลูปแล้วเรียก Callback กลับไปทีละรายการ
            for (int i = 0; i < n; ++i) {
                String ssid = WiFi.SSID(i);
                int32_t rssi = WiFi.RSSI(i);
                bool isEncrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                onNetworkScanned(true, i + 1, n, ssid, rssi, isEncrypted);
                delay(10);
            }
        }
        // ล้างผลการสแกนเพื่อคืนหน่วยความจำ
        WiFi.scanDelete();
    }
    
    const char* get_token() {
        return token;
    }
    void RPCRoute(const RPC_Callback* RPC_INPUT){
                std::copy(RPC_INPUT, RPC_INPUT + MAX_RPC_SUBSCRIPTIONS, callbacks);    
        }
    void begin(){createTxTask(); }
    bool get_wifiStatus(){
      return WIFI_connected;
    }
    bool get_rpcStatus(){
      return RPC_subscribed;
    }
    bool get_thingsboardStatus(){
      return TB_connected;
    }
    String get_SSID(){
      return WIFI_connected? SSID : "disconnected";;
    }
    String get_ipAddr(){
      return WIFI_connected? localIP : "disconnected";
    }

      template <typename T>
      void sendTelemetryData(const String& key, const T& data) {
          // if (!TB_connected) return;
          // Convert the String to a C-string (const char*)
          TBClient.sendTelemetryData(key.c_str(), data);
      }

      template <typename T>
      void sendAttributeData(const String& key, const T& data) {
          // if (!TB_connected) return;
          TBClient.sendAttributeData(key.c_str(), data);
          }
  };
  #endif