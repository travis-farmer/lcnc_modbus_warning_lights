/**
 * \brief Arduino ModBus project, controlling status lights.
 * The master/client port will need to be configured using the following settings:
 * Baud Rate: 9600
 * Data Bits: 8
 * Parity: None
 * Stop Bit(s): 1
 * Slave/Server ID: 0
 */

#include <ModbusRTUSlave.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#define PIN        6
#define NUMPIXELS 3

byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xEC };
IPAddress server(10, 150, 86, 185);
IPAddress ip(192, 168, 0, 29);
IPAddress myDns(192, 168, 0, 1);


Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ800);

const int dePin = 5;

const byte id = 0;
const unsigned long baud = 9600;
const unsigned int bufSize = 256;

const unsigned int numCoils = 5;
const unsigned int numDiscreteInputs = 0;
const unsigned int numHoldingRegisters = 6;
const unsigned int numInputRegisters = 0;

bool lcncProgramRunning = false;
bool lcncProgramIdle = false;
bool lcncProgramPaused = false;
bool lcncEstop = false;
bool lcncLaserRunning = false;
unsigned long lastTimer = 0UL;
unsigned long lastTimerB = 0UL;
int cycle_hours = 0;
int cycle_mins = 0;
int cycle_secs = 0;
int lcnc_hours = 0;
int lcnc_mins = 0;
int lcnc_secs = 0;

byte buf[bufSize];
ModbusRTUSlave modbus(Serial1, buf, bufSize, dePin);

/********************************************//**
 * \brief MQTT callback
 *
 * \param topic char*
 * \param payload byte*
 * \param length unsigned int
 * \return void
 *
 ***********************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  String tmpTopic = topic;
  char tmpStr[length+1];
  for (int x=0; x<length; x++) {
    tmpStr[x] = (char)payload[x]; // payload is a stream, so we need to chop it by length.
  }
  tmpStr[length] = 0x00; // terminate the char string with a null

  /*if (tmpTopic == "weather/winddir") {int intWindDir = atoi(tmpStr); gblWindDir = (moduloWindDir(intWindDir + 90)); }
  else if (tmpTopic == "weather/windspeedmph") { gblWindSpeed = atof(tmpStr); }
  else if (tmpTopic == "weather/humidity") { gblHumidity = atof(tmpStr); }
  else if (tmpTopic == "weather/tempf") { gblTempF = atof(tmpStr); }
  else if (tmpTopic == "weather/rainin") { gblRainIn = atof(tmpStr); }
  else if (tmpTopic == "weather/dailyrainin") { gblDRainIn = atof(tmpStr); }
  else if (tmpTopic == "weather/batt_lvl") { gblBattLvl = atof(tmpStr); }
  //else if (tmpTopic == "weather/altitude") { gblAltitude = atoi(tmpStr); }
  else if (tmpTopic == "weather/light_lvl") { gblLightLvl = atof(tmpStr); }
  else if (tmpTopic == "weather/lat") { gblGpsLat = tmpStr; }
  else if (tmpTopic == "weather/lng") { gblGpsLong = tmpStr; }
  else if (tmpTopic == "weather/sats") { gblGpsSats = tmpStr; }
  else if (tmpTopic == "weather/date") { gblGpsDate = tmpStr; }
  else if (tmpTopic == "weather/time") { gblGpsTime = tmpStr; }

  else if (tmpTopic == "generator/Status/Engine/Engine_State") { gblGenEngState = tmpStr; }
  else if (tmpTopic == "generator/Status/Engine/Switch_State") { gblGenEngSwitch = tmpStr; }
  else if (tmpTopic == "generator/Status/Engine/Battery_Voltage") { gblGenEngBatt = tmpStr; } */

}

EthernetClient eClient;
PubSubClient client(eClient);

long lastReconnectAttempt = 0;

/********************************************//**
 * \brief reconnect
 *
 * \return boolean
 *
 ***********************************************/
boolean reconnect() {
  if (client.connect("arduinoClient")) {
    // Once connected, publish an announcement...
    // client.publish("test/outTopic","testing");
    // ... and resubscribe
    // client.subscribe("generator/Monitor/Platform_Stats/System_Uptime");
    //client.subscribe("weather/winddir");

  }
  return client.connected();
}

/**
 * \brief Arduino Setup
 *
 * \return void
 *
 */
void setup() {
    Serial1.begin(baud);
    modbus.begin(id, baud);
    modbus.configureCoils(numCoils, coilRead, coilWrite);
    modbus.configureDiscreteInputs(numDiscreteInputs, discreteInputRead);
    modbus.configureHoldingRegisters(numHoldingRegisters, holdingRegisterRead, holdingRegisterWrite);
    modbus.configureInputRegisters(numInputRegisters, inputRegisterRead);


    client.setServer(server, 1883);
    client.setCallback(callback);
    //Serial.println("Initialize Ethernet with DHCP:");
    if (Ethernet.begin(mac) == 0) {
        //Serial.println("Failed to configure Ethernet using DHCP");
        // Check for Ethernet hardware present
        if (Ethernet.hardwareStatus() == EthernetNoHardware) {
            //Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
            errorProc(1);
        }
        if (Ethernet.linkStatus() == LinkOFF) {
            //Serial.println("Ethernet cable is not connected.");
            errorProc(2);
        }
        // try to congifure using IP address instead of DHCP:
        Ethernet.begin(mac, ip, myDns);
    } else {
        //Serial.print("  DHCP assigned IP ");
        //Serial.println(Ethernet.localIP());
    }
    delay(1500);
    lastReconnectAttempt = 0;

    pixels.begin();
    pixels.clear();
}

/**
 * \brief Arduino Loop
 *
 * \return void
 *
 */
void loop() {
    modbus.poll();

    if (!client.connected()) {
        long now = millis();
        if (now - lastReconnectAttempt > 5000) {
                lastReconnectAttempt = now;
                // Attempt to reconnect
            if (reconnect()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        // Client connected

        client.loop();
    }
    int lightStatus = 0;
    int lightRun = 0;
    int lightLaser = 0;
    if (millis() - lastTimer >= 50) {
        lastTimer = millis();
        if (lcncProgramRunning == true && lcncProgramPaused == false && lcncProgramIdle == false && lcncEstop == true) {
            pixels.setPixelColor(0, pixels.Color(0,255,0)); // green
            pixels.setPixelColor(1, pixels.Color(0,255,0));
            lightStatus = 1;
            pixels.setPixelColor(2, pixels.Color(255,255,255)); // white
            pixels.setPixelColor(3, pixels.Color(255,255,255));
            lightRun = 1;
        } else if (((lcncLaserRunning == true && lcncProgramPaused == true) || lcncProgramIdle == true) && lcncEstop == true) {
            pixels.setPixelColor(0, pixels.Color(255,255,0)); // orange
            pixels.setPixelColor(1, pixels.Color(255,255,0));
            lightStatus = 2;
            pixels.setPixelColor(2, pixels.Color(0,0,0)); // off
            pixels.setPixelColor(3, pixels.Color(0,0,0));
            lightRun = 0;
        } else if (lcncEstop == false) {
            pixels.setPixelColor(0, pixels.Color(255,0,0)); // red
            pixels.setPixelColor(1, pixels.Color(255,0,0));
            lightStatus = 3;
            pixels.setPixelColor(2, pixels.Color(0,0,0)); // off
            pixels.setPixelColor(3, pixels.Color(0,0,0));
            lightRun = 0;
        }
        if (lcncLaserRunning == true) {
            pixels.setPixelColor(4, pixels.Color(0,0,255)); // blue
            pixels.setPixelColor(5, pixels.Color(0,0,255));
            lightLaser = 1;
        } else {
            pixels.setPixelColor(4, pixels.Color(0,0,0)); // off
            pixels.setPixelColor(5, pixels.Color(0,0,0));
            lightLaser = 0;
        }
        pixels.show();

        char sz[32];
        sprintf(sz, "%d", lightStatus);
        client.publish("lcnc/lightstatus",sz);
        sprintf(sz, "%d", lightRun);
        client.publish("lcnc/lightrun",sz);
        sprintf(sz, "%d", lightLaser);
        client.publish("lcnc/lightlaser",sz);
        sprintf(sz, "%d", lcncProgramIdle? 1:0);
        client.publish("lcnc/progidle",sz);
        sprintf(sz, "%d", lcncProgramRunning? 1:0);
        client.publish("lcnc/progrunning",sz);
        sprintf(sz, "%d", lcncProgramPaused? 1:0);
        client.publish("lcnc/progpaused",sz);
        sprintf(sz, "%d", lcncLaserRunning? 1:0);
        client.publish("lcnc/laserrunning",sz);
        sprintf(sz, "%d", lcncEstop? 1:0);
        client.publish("lcnc/estop",sz);
        sprintf(sz, "%d:%d:%d", cycle_hours, cycle_mins, cycle_secs);
        client.publish("lcnc/cycletime",sz);
        sprintf(sz, "%d:%d:%d", lcnc_hours, lcnc_mins, lcnc_secs);
        client.publish("lcnc/lcnctime",sz);
    }
}



/**
 * \brief ModBus Coil Read handle
 *
 * \param address unsigned int
 * \return char
 *
 */
char coilRead(unsigned int address) {
    // mb2hal: fnct_01_read_coils
    // return 0 or 1

}

/**
 * \brief ModBus Coil Write Handle
 *
 * \param address unsigned int
 * \param value boolean
 * \return boolean
 *
 */
boolean coilWrite(unsigned int address, boolean value) {
    // mb2hal: fnct_05_write_single_coil
    switch(address) {
    case 0:
        lcncProgramRunning = value;
        break;
    case 1:
        lcncProgramIdle = value;
        break;
    case 2:
        lcncProgramPaused = value;
        break;
    case 3:
        lcncEstop = value;
        break;
    case 4:
        lcncLaserRunning = value;
        break;
    }

  return true;
}

/**
 * \brief ModBus Discrete Input Read Handle
 *
 * \param address unsigned int
 * \return char
 *
 */
char discreteInputRead(unsigned int address) {
    // mb2hal: fnct_02_read_discrete_inputs

}

/**
 * \brief ModBus Holding Register Read Handle
 *
 * \param address unsigned int
 * \return long
 *
 */
long holdingRegisterRead(unsigned int address) {
    // mb2hal: fnct_03_read_holding_registers
    //return ads.readADC_SingleEnded(address);
}

/**
 * \brief ModBus Holding Register Write Handle
 *
 * \param address word
 * \param value word
 * \return boolean
 *
 */
boolean holdingRegisterWrite(word address, word value) {
    // mb2hal: fnct_16_write_multiple_registers
    switch(address) {
    case 0:
        cycle_hours = (int)value;
        break;
    case 1:
        cycle_mins = (int)value;
        break;
    case 2:
        cycle_secs = (int)value;
        break;
    case 3:
        lcnc_hours = (int)value;
        break;
    case 4:
        lcnc_mins = (int)value;
        break;
    case 5:
        lcnc_secs = (int)value;
        break;
    }
  return true;
}

/**
 * \brief ModBus Input Register Read Handle
 *
 * \param address word
 * \return long
 *
 */
long inputRegisterRead(word address) {
    // mb2hal: fnct_04_read_input_registers
    // return long
}
