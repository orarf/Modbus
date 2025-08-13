#include<WiFi.h>
#include<ThingsBoard.h>
#include<Arduino_MQTT_Client.h>
#include<ArduinoJson.h>
#define MAX_Fields_Amount 1
#define Max_Saved_Telemetry 10
#define MAX_Attributes_Amount 1
#define Max_Saved_Attributes 10
#define RPC_Callback_MEMBER 6
#define TBmanager_Stack_Size 1024*10
class TBmanager{
    private:
        const char*a;const char*b;const char*c;const uint16_t d;const char*e;const uint16_t f=255U;bool g=false;WiFiClient h;Arduino_MQTT_Client i;ThingsBoardSized<MAX_Fields_Amount,Max_Saved_Telemetry,MAX_Attributes_Amount,Max_Saved_Attributes>j;std::vector<RPC_Callback>k;std::array<RPC_Callback,RPC_Callback_MEMBER>l;
        void m(){WiFi.begin(a,b);while(WiFi.status()!=WL_CONNECTED){vTaskDelay(pdMS_TO_TICKS(1000));}#ifdef DEBUG Serial.println("WiFi connected [ok]");Serial.print("Connected to: ");Serial.println(WiFi.SSID());Serial.print("IP address: ");Serial.println(WiFi.localIP());#endif}
        void n(){if(WiFi.status()!=WL_CONNECTED){#ifdef DEBUG Serial.println("WiFi disconnected, reconnecting...");#endif m();}o();}
        void o(){g=j.connected();if(!g){#ifdef DEBUG Serial.println("Connecting to ThingsBoard server");#endif if(!j.connect(c,e,d)){#ifdef DEBUG Serial.println("ThingsBoard Connected [Failed]");#endif vTaskDelay(pdMS_TO_TICKS(1000));}else{#ifdef DEBUG Serial.println("ThingsBoard Connected [ok]");#endif j.RPC_Unsubscribe();if(k.size()>0){if(!j.RPC_Subscribe(l.cbegin(),l.cend())){#ifdef DEBUG Serial.println("RPC subscribed [Failed]");#endif vTaskDelay(pdMS_TO_TICKS(1000));j.RPC_Unsubscribe();j.disconnect();}else{#ifdef DEBUG Serial.println("RPC subscribed [ok]");#endif}}}}
        static void p(void*q){static_cast<TBmanager*>(q)->q(q);}
        void r(){if(xTaskCreatePinnedToCore(p,"Task_TBmanager",TBmanager_Stack_Size,this,5,NULL,1)!=pdPASS){#ifdef DEBUG printf("Failed to create Task_TBmanager\n");#endif}}
        void q(void*pvParameters){m();o();j.sendAttributeData("IP_Address",(String)WiFi.localIP());j.sendAttributeData("Connected_SSID",(String)WiFi.SSID());while(true){n();j.loop();vTaskDelay(pdMS_TO_TICKS(500));}}
    public:
        TBmanager(const char*ss,const char*pa,const char*se,const uint16_t po,const char*to):a(ss),b(pa),c(se),d(po),e(to),i(h),j(i,f){}
        template<typename fnc>void RPCRoute(char const*const mn,fnc cf){k.push_back(RPC_Callback{mn,cf});}
        ~TBmanager(){j.RPC_Unsubscribe();j.disconnect();}
        void begin(){std::copy(k.begin(),k.end(),l.begin());r();}
        template<typename ntype>void sendTelemetryData(const char*key,const ntype&data){if(!g)return;j.sendTelemetryData(key,data);}
        template<typename ntype>void sendAttributeData(const char*key,const ntype&data){if(!g)return;j.sendAttributeData(key,data);}
};
uint16_t packCharacters(char c1,char c2){return(static_cast<uint16_t>(c1)<<8)|static_cast<uint16_t>(c2);}
void unpackCharacters(uint16_t packed,char&c1,char&c2){c1=static_cast<char>((packed>>8)&0xFF);c2=static_cast<char>(packed&0xFF);}
void encode(const String&input,uint16_t*output){size_t length=input.length();output[0]=length;size_t outIndex=1;for(size_t i=0;i<length;i+=2){char c1=input[i];char c2=(i+1<length)?input[i+1]:0;output[outIndex++]=packCharacters(c1,c2);}}
void decode(const uint16_t*input,String&output){size_t length=input[0];output="";for(size_t i=1;i<=(length+1)/2;++i){char c1,c2;unpackCharacters(input[i],c1,c2);output+=c1;if(c2!=0){output+=c2;}}}
