/**
 * \brief Arduino ModBus project, controlling a automatic tool changer.
 * The master/client port will need to be configured using the following settings:
 * Baud Rate: 9600
 * Data Bits: 8
 * Parity: None
 * Stop Bit(s): 1
 * Slave/Server ID: 0
 */

#include <ModbusRTUSlave.h>
#include <Adafruit_NeoPixel.h>

#define PIN        6
#define NUMPIXELS 3
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ800);

const int dePin = 12;

const byte id = 0;
const unsigned long baud = 9600;
const unsigned int bufSize = 256;

const unsigned int numCoils = 5;
const unsigned int numDiscreteInputs = 0;
const unsigned int numHoldingRegisters = 0;
const unsigned int numInputRegisters = 0;

bool lcncProgramRunning = false;
bool lcncProgramIdle = false;
bool lcncProgramPaused = false;
bool lcncEstop = false;
bool lcncLaserRunning = false;
unsigned long lastTimer = 0UL;

byte buf[bufSize];
ModbusRTUSlave modbus(Serial1, buf, bufSize, dePin);


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
    if (millis() - lastTimer >= 50) {
        lastTimer = millis();
        if (lcncProgramRunning == true && lcncProgramPaused == false && lcncProgramIdle == false && lcncEstop == true) {
            pixels.setPixelColor(0, pixels.Color(0,255,0)); // green
            pixels.setPixelColor(1, pixels.Color(0,255,0));
            pixels.setPixelColor(2, pixels.Color(255,255,255)); // white
            pixels.setPixelColor(3, pixels.Color(255,255,255));
        } else if (((lcncLaserRunning == true && lcncProgramPaused == true) || lcncProgramIdle == true) && lcncEstop == true) {
            pixels.setPixelColor(0, pixels.Color(255,255,0)); // orange
            pixels.setPixelColor(1, pixels.Color(255,255,0));
            pixels.setPixelColor(2, pixels.Color(0,0,0)); // off
            pixels.setPixelColor(3, pixels.Color(0,0,0));
        } else if (lcncEstop == false) {
            pixels.setPixelColor(0, pixels.Color(255,0,0)); // red
            pixels.setPixelColor(1, pixels.Color(255,0,0));
            pixels.setPixelColor(2, pixels.Color(0,0,0)); // off
            pixels.setPixelColor(3, pixels.Color(0,0,0));
        }
        if (lcncLaserRunning == true) {
            pixels.setPixelColor(4, pixels.Color(0,0,255)); // blue
            pixels.setPixelColor(5, pixels.Color(0,0,255));
        }
        pixels.show();
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
