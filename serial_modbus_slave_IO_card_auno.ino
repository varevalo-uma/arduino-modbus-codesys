/**
    @file serial_modbus_slave_io_card_auno.ino
    @version 3
    @author Vicente Arévalo

    This software enables arduino to work as a modbus slave io card with:
      - 6 digital inputs/outputs
    by using a RS232 connection for the communication with the master.

    RS232 setup:
    ----------------------------------
    |      Parameter     |   Value   |
    ----------------------------------
    |  Slave ID          |       1   |
    |  Bauds             |  115200   |
    |  Num of bits       |       8   |
    |  Parity            |    NONE   |
    |  Num of stop bits  |       1   |
    ----------------------------------

    I/O <pin - register> map:
    ------------------------------------------------------------------------------------
    |        Input/Output        |           Arduino            |    Modbus register   |
    ------------------------------------------------------------------------------------
    |  Digital input  (FC 0x02)  |  A0, A1,  A2,  A3,  A4,  A5  |   0, 1, 2, 3, 4, 5   |
    |  Digital output (FC 0x05)  |  D8, D9, D10, D11, D12, D13  |   0, 1, 2, 3, 4, 5   |
    ------------------------------------------------------------------------------------
    ----------------------------------------
    | Modbus FC |        Operation         |
    ----------------------------------------
    |   0x02    |   Read Discrete Inputs   |
    |   0x05    |   Write Single Coil      |
    ----------------------------------------
*/

#include <ModbusSerial.h>

// IO card data
const byte SlaveId = 1;
const unsigned long BaudRate = 115200;
const int TxenPin = -1; // -1 disables the feature, change that if you are using an RS485 driver, this pin would be connected to the DE and /RE pins of the driver.

// IO map (arduino pin <-> modbus register)
struct ioMap { int reg; int pin; };
ioMap digitalInput[]  = { {0, A0}, {1, A1}, {2, A2}, {3, A3}, {4, A4}, {5, A5} };
ioMap digitalOutput[] = { {0,  8}, {1,  9}, {2, 10}, {3, 11}, {4, 12}, {5, 13} };

// modbus master communication
#define MySerial Serial // define serial port used, Serial most of the time, or Serial1, Serial2 ... if it is available

// modbus serial object
ModbusSerial Modbus(MySerial, SlaveId, TxenPin);

//  setup the modbus serial object & I/O registers
void setup_modbus()
{
  MySerial.begin(BaudRate); // 8 bits, none parity, 1 stop bit
  while (!MySerial);

  Modbus.config(BaudRate);
  Modbus.setAdditionalServerData("Serial MODBUS IO card"); // for Report Server ID function (0x11)

  // setup registers for analog/digital inputs/outputs
  for (int i = 0; i < 6; i++)
  {
    Modbus.addIsts(digitalInput[i].reg);
    Modbus.addCoil(digitalOutput[i].reg);
  }
}

//  setup the arduino I/O pines
void setup_pines()
{
  for (int i = 0; i < 6; i++)
  {
    pinMode(digitalInput[i].pin,  INPUT);
    pinMode(digitalOutput[i].pin, OUTPUT);
  }
}

void myDigitalWrite(int idx)
{
  bool val = Modbus.Coil(digitalOutput[idx].reg);
  digitalWrite(digitalOutput[idx].pin, val);
}

void myDigitalRead(int idx)
{
  bool val = digitalRead(digitalInput[idx].pin);
  Modbus.Ists(digitalInput[idx].reg, val);
}

void setup()
{
  setup_modbus();
  setup_pines();
}

void loop()
{
  // Call once inside loop() - all magic here
  Modbus.task();

  // read & write data from / to the arduino inputs / outputs
  for (int i = 0; i < 6; i++)
  {
    myDigitalWrite(i);
    myDigitalRead(i);
  }
}

