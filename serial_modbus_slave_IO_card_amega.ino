/**
    @file serial_modbus_slave_io_card.ino
    @version 3
    @author Vicente Arévalo

    This software enables arduino to work as a modbus slave with:
      - 8 analog/digital inputs/outputs and,
      - 2 SG-90 servo motors outputs (0-180º range)
    by using a RS232 connection for the communication with the master.
    
    Note: Remember!, the standard resolution of the arduino analog functions: 
             - analogRead = 10 bits, and 
             - analogWrite = 8 bits. 
          So, this code properly scales input/output data to/from 2 bytes words
          for correctly working.

    RS232 setup:
    ----------------------------------
    |      Parameter     |   Value   |
    ----------------------------------
    |  Slave ID          |       1   |
    |  Bauds             |  115200   |
    |  Parity            |    NONE   |
    |  Num of bits       |       8   |
    |  Num of stop bits  |       1   |
    ----------------------------------

    I/O <pin - register> map:
    ---------------------------------------------------------------------------------------------
    |        Input/Output        |             Arduino              |      Modbus register      |
    ---------------------------------------------------------------------------------------------
    |  Analog input   (FC 0x04)  |  A0, A1, A2, A3, A4, A5, A6, A7  |   0, 1, 2, 3, 4, 5, 6, 7  |
    |  Analog output  (FC 0x06)  |   2,  3,  4,  5,  6,  7,  8,  9  |   0, 1, 2, 3, 4, 5, 6, 7  |
    |  Digital input  (FC 0x02)  |  62, 63, 64, 65, 66, 67, 68, 69  |   0, 1, 2, 3, 4, 5, 6, 7  |
    |  Digital output (FC 0x05)  |  23, 25, 27, 29, 31, 33, 35, 37  |   0, 1, 2, 3, 4, 5, 6, 7  |
    ---------------------------------------------------------------------------------------------
    |  Servo          (FC 0x06)  |  10, 11                          |   10, 11                  |
    ---------------------------------------------------------------------------------------------
    ----------------------------------------
    | Modbus FC |        Operation         |
    ----------------------------------------
    |   0x02    |   Read Discrete Inputs   |
    |   0x04    |   Read Input Registers   |
    |   0x05    |   Write Single Coil      |
    |   0x06    |   Write Single Register  |
    ----------------------------------------
*/

#include <Servo.h>
#include <ModbusSerial.h>

// IO card data
const byte SlaveId = 1;
const unsigned long BaudRate = 115200;

// IO map (arduino pin <-> modbus register)
struct ioMap { int pin; int reg; };
ioMap servoOutput[]   = { {10,10}, {11,11} };
ioMap analogInput[]   = { {A0, 0}, {A1, 1}, {A2, 2}, {A3, 3}, {A4, 4}, {A5, 5}, {A6, 6}, {A7, 7} };
ioMap analogOutput[]  = { { 2, 0}, { 3, 1}, { 4, 2}, { 5, 3}, { 6, 4}, { 7, 5}, { 8, 6}, { 9, 7} };
ioMap digitalInput[]  = { {62, 0}, {63, 1}, {64, 2}, {65, 3}, {66, 4}, {67, 5}, {68, 6}, {69, 7} };
ioMap digitalOutput[] = { {23, 0}, {25, 1}, {27, 2}, {29, 3}, {31, 4}, {33, 5}, {35, 6}, {37, 7} };

Servo servo[2];

const int TxenPin = -1; // -1 disables the feature, change that if you are using an RS485 driver, this pin would be connected to the DE and /RE pins of the driver.

// modbus master communication
#define MySerial Serial // define serial port used, Serial most of the time, or Serial1, Serial2 ... if it is available

// modbus serial object
ModbusSerial mb(MySerial, SlaveId, TxenPin);

//  setup the modbus serial object & I/O registers
void setup_modbus()
{
  MySerial.begin(BaudRate); // 8 bits, none parity, 1 stop bit
  while(!MySerial);

  mb.config(BaudRate);
  mb.setAdditionalServerData("Serial MODBUS IO card"); // for Report Server ID function (0x11)

  // setup registers for analog/digital inputs/outputs
  for (int i = 0; i < 8; i++)
  {
    mb.addIreg(analogInput[i].reg);
    mb.addHreg(analogOutput[i].reg);
    mb.addIsts(digitalInput[i].reg);
    mb.addCoil(digitalOutput[i].reg);
  }

  // setup resgisters for servo motors
  for (int i = 0; i < 2; i++)
    mb.addHreg(servoOutput[i].reg);
}

//  setup the arduino I/O pines
void setup_pines()
{
  for (int i = 0; i < 8; i++)
  {
    pinMode(digitalInput[i].pin,  INPUT);
    pinMode(digitalOutput[i].pin, OUTPUT);
  }

  for(int i = 0; i < 2; i++)
      servo[i].attach(servoOutput[i].pin);
}

void servoWrite(int idx)
{
  word val = mb.Hreg(servoOutput[idx].reg);
  servo[idx].write(map(val, 0, 65535, 0, 180)); // word to 0 - 180 degrees
}

void myAnalogWrite(int idx)
{
  word val = mb.Hreg(analogOutput[idx].reg);
  analogWrite(analogOutput[idx].pin, map(val, 0, 65535, 0, 255)); // word (2 bytes) to byte mapping
}

void myAnalogRead(int idx)
{
  int val = analogRead(analogInput[idx].pin);
  mb.Ireg(analogInput[idx].reg, map(val, 0, 1023, 0, 65535)); // 10-bits to word (2 bytes) mapping
}

void myDigitalWrite(int idx)
{
  bool val = mb.Coil(digitalOutput[idx].reg);
  digitalWrite(digitalOutput[idx].pin, val);
}

void myDigitalRead(int idx)
{
  bool val = digitalRead(digitalInput[idx].pin);
  mb.Ists(digitalInput[idx].reg, val);
}

void setup()
{
  setup_modbus();
  setup_pines();
}

void loop()
{
  // Call once inside loop() - all magic here
  mb.task();

  // read & write data from / to the arduino inputs / outputs
  for(int i = 0; i < 8; i++)
  {
    myAnalogWrite(i);
    myAnalogRead(i);
    myDigitalWrite(i);
    myDigitalRead(i);
  }

  // write data to the arduino servo outputs
  for(int i = 0; i < 2; i++)
    servoWrite(i);
}

