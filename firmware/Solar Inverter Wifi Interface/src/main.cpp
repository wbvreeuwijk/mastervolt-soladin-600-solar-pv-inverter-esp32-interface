#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <Soladin.h>
#include "main.h"

#define RXD2 16
#define TXD2 17

// #define DEBUG // When DEBUG is defined log text will be sent to the UART programming port.

#ifdef DEBUG
    #define TRACE(x) Serial.print(x);
#else
    #define TRACE(x)
#endif

#define SLEEP_TIME 10000000  				//ESP32 will sleep for 10 seconds

/* Put your SSID & Password */
const char *ssid = "<SSID>>";        // Enter SSID here
const char *password = "<PASSWORD>"; 			//Enter Password here

// mqtt client

const char *mqtt_server = "<MQTTSERVER>";	// MQTT Server Address here
const char *mqtt_user = "<MQTTUSER>";	// MQTT Username here
const char *mqtt_password = "<MQTTPASSWORD>";		// MQTT Password here

WiFiClient espClient;
PubSubClient client(espClient);

// soladin
Soladin sol ;						 		// instance of soladin class
boolean solconnected = false ;			    // soladin connection status

void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        #ifdef DEBUG
            TRACE("Attempting MQTT connection...\n");
        #endif
        // Attempt to connect
        if (client.connect("SolarInverterClient", mqtt_user, mqtt_password)) {
            #ifdef DEBUG
                TRACE("connected\n");
            #endif
        }
        else {
            #ifdef DEBUG
                TRACE("failed, rc=");
                TRACE(client.state());
                TRACE(" try again in 5 seconds\n");
            #endif
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void uploaddata(void) {
    if (solconnected && client.connected()) {
        if (sol.query(DVS)) {				// request Device status
            TRACE("Getting Device Status\n");
            if (sol.Flag != 0x00) {				// Print error flags
                TRACE("Error flags found\n");
                client.publish("/home/solarpv/alarmflag", String(sol.Flag).c_str(), true);
            }
            else{
                TRACE("Build CSV Object");
                String data = "pvvolt="+String(float(sol.PVvolt)/10);
                data += ";pvamp="+String(float(sol.PVamp)/100);
                data += ";gridvolt=" + String(sol.Gridvolt);
                data += ";gridpower=" + String(sol.Gridpower);
                data += ";gridfrequency=" + String(float(sol.Gridfreq)/100);
                data += ";totalpower=" + String(float(sol.Totalpower)/100);
                data += ";devicetemp=" + String(sol.DeviceTemp);
                char timeStr[14];
                sprintf(timeStr, "%04d:",(sol.TotalOperaTime/60));
                data += ";operationtime=" + String(timeStr);
                data += ";alarmflag=" + String(sol.Flag);
                TRACE("Sending CSV Messages\n");
                client.publish("home/solarpv", data.c_str());
                /* upload data to the mqtt client
                client.publish("/home/solarpv/pvvolt", String(float(sol.PVvolt)/10).c_str(), true);
                client.publish("/home/solarpv/pvamp", String(float(sol.PVamp)/100).c_str(), true);
                client.publish("/home/solarpv/gridvolt", String(sol.Gridvolt).c_str(), true);
                client.publish("/home/solarpv/gridpower", String(sol.Gridpower).c_str(), true);
                client.publish("/home/solarpv/gridfrequency", String(float(sol.Gridfreq)/100).c_str(), true);
                client.publish("/home/solarpv/totalpower", String(float(sol.Totalpower)/100).c_str(), true);
                client.publish("/home/solarpv/devicetemp", String(sol.DeviceTemp).c_str(), true);
                char timeStr[14];
                sprintf(timeStr, "%04d:",(sol.TotalOperaTime/60));
                client.publish("/home/solarpv/operationtime", String(timeStr).c_str(), true);
                client.publish("/home/solarpv/alarmflag", String(sol.Flag).c_str(), true);
                */
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    //Set sleep timer
	esp_sleep_enable_timer_wakeup(SLEEP_TIME);

    // Connect to WiFi
    TRACE("\nConnecting to Wifi\n");
    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, password);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        TRACE(".");
    }
    TRACE("\n");
    TRACE("Connected to ");
    TRACE(ssid);
    TRACE("\nIP address: ");
    TRACE(WiFi.localIP());
    TRACE("\nMAC address: ");
    TRACE(WiFi.macAddress());
    TRACE("\n");

    // set up mqtt client

	client.setServer(mqtt_server, 1883);

    // set up soladin
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
    sol.begin(&Serial2);

    // soladin connect loop
    while (!solconnected) {      					// Try to connect
        TRACE("Cmd: Probe");
        for (int i=0 ; i < 4 ; i++) {
        if (sol.query(PRB)) {				// Try connecting to slave
            solconnected = true;
            TRACE("...Connected");
            break;
        }
        TRACE(".");
        delay(1000);
        }
    }
}

void loop() {
    if (!client.connected()) {
		reconnect();
	}
	client.loop();

    uploaddata();

    //Go to sleep now
	delay(10000);
}