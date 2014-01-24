#include <SPI.h>

int led = 13;
int _slaveSelectPin = 10; 
String content = "";
char character;
int dio0 = 3;
int dio5 = 2;
int  receiving = 1;
byte currentMode = 0x81;

#define REG_FIFO                    0x00
#define REG_FIFO_ADDR_PTR           0x0D 
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F
#define REG_RX_NB_BYTES             0x1D
#define RF92_MODE_RX_CONTINUOS      0x85
#define RF92_MODE_TX                0x83
#define RF92_MODE_SLEEP             0x80
#define RF92_MODE_STANDBY           0x81
#define REG_OPMODE                  0x01
#define REG_RX_DATA_ADDR            0x26
#define REG_IRQ_FLAGS               0x10
#define REG_DIO_MAPPING_1           0x40
#define REG_DIO_MAPPING_2           0x41
#define REG_SYMB_TIMEOUT            0x14

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the pins
  pinMode( _slaveSelectPin, OUTPUT);
  pinMode(led, OUTPUT); 
  pinMode(dio0, INPUT);
  pinMode(dio5, INPUT);
  Serial.begin(9600);
  SPI.begin();

  delay(3000);  // Wait for me to open serial monitor

  // LoRa mode 
  setLoRaMode();
  
  // Turn on implicit header mode
  writeRegister(REG_SYMB_TIMEOUT,0x0C);
  
  // Setup Receive Continous Mode
  setMode(RF92_MODE_RX_CONTINUOS);

  Serial.println(readRegister(REG_OPMODE),HEX);
  Serial.println("Setup Complete");
}

// the loop routine runs over and over again forever:
void loop() {  
  
  if(digitalRead(dio0) == 1 && receiving == 0)
  {
    receiveMessage();
  }
   
  while (Serial.available() > 0) {
     // read the incoming byte:
    character = (char)Serial.read();
    content.concat(character);
  
    if (!Serial.available()) {
      if(content == "1"){
        Serial.println("Reading all registers");
        readAllRegs();
      }
      else if (content == "2"){
       receiving = 0;
       Serial.println("Entering Receiving Mode");
      }  
      else if (content == "3"){
        Serial.print("DIO0 value is: ");
        Serial.println(digitalRead(dio0));
      }
      else if (content == "4"){
        Serial.print("DIO5 value is: ");
        Serial.println(digitalRead(dio5));
      }
  
      content = "";
    } 
  }
}

/////////////////////////////////////
//    Method:   Receive FROM BUFFER
//////////////////////////////////////
void receiveMessage()
{
  receiving = 1;
  Serial.println("Packet Received");
  int x = readRegister(REG_IRQ_FLAGS); // if any of these are set then the inbound message failed
  if(x != 0){
    Serial.println("Oops there was a problem!!");
    Serial.println(x);
    receiving = 0;
  }
  else{
     
    byte value = readRegister(REG_RX_DATA_ADDR);
    Serial.print("Value: ");
    Serial.println(value,HEX);
    
    byte receivedCount = readRegister(REG_RX_NB_BYTES);
    byte loc = value - receivedCount;
    Serial.print("loc: ");
    Serial.println(loc,HEX);

    writeRegister(REG_FIFO_ADDR_PTR, loc);   
    Serial.print("Number of bytes received:");
    Serial.println(receivedCount);
    for(int i = 0; i < receivedCount; i++)
    {
      Serial.println(readRegister(REG_FIFO)); 
    }
    
    // blink the LED
    digitalWrite(led, HIGH);
    delay(100);              
    digitalWrite(led, LOW);
  } 
}

/////////////////////////////////////
//    Method:   Read Register
//////////////////////////////////////

byte readRegister(byte addr)
{
  select();
  SPI.transfer(addr & 0x7F);
  byte regval = SPI.transfer(0);
  unselect();
  return regval;
}

/////////////////////////////////////
//    Method:   Write Register
//////////////////////////////////////

void writeRegister(byte addr, byte value)
{
  select();
  SPI.transfer(addr | 0x80); // OR address with 10000000 to indicate write enable;
  SPI.transfer(value);
  unselect();
}

/////////////////////////////////////
//    Method:   Select Transceiver
//////////////////////////////////////
void select() 
{
  digitalWrite(_slaveSelectPin, LOW);
}

/////////////////////////////////////
//    Method:   UNSelect Transceiver
//////////////////////////////////////
void unselect() 
{
  digitalWrite(_slaveSelectPin, HIGH);
}

/////////////////////////////////////
//    Method:   Read ALL Registers
//////////////////////////////////////
void readAllRegs( )
{
  byte regVal;
        
  for (byte regAddr = 1; regAddr <= 0x46; regAddr++)
  {
    select();
    SPI.transfer(regAddr & 0x7f);        // send address + r/w bit
    regVal = SPI.transfer(0);
    unselect();
  
    Serial.print(regAddr, HEX);
    Serial.print(" - ");
    Serial.print(regVal,HEX);
    Serial.print(" - ");
    Serial.println(regVal,BIN);
  }
}

/////////////////////////////////////
//    Method:   Change the mode
//////////////////////////////////////
void setMode(byte newMode)
{
  if(newMode == currentMode)
    return;  
  
  switch (newMode) 
  {
    case RF92_MODE_RX_CONTINUOS:
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      Serial.println("Changing to Receive Continous Mode");
      break;
    case RF92_MODE_TX:
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      Serial.println("Changing to Transmit Mode");
      break;
    case RF92_MODE_SLEEP:
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      Serial.println("Changing to Sleep Mode"); 
      break;
    case RF92_MODE_STANDBY:
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      Serial.println("Changing to Standby Mode");
      break;
    default: return;
  } 
  
  if(newMode != RF92_MODE_SLEEP){
    while(digitalRead(dio5) == 0)
    {
      Serial.print("z");
    } 
  }
  
  Serial.println(" Mode Change Done");
  return;
}

/////////////////////////////////////
//    Method:   Enable LoRa mode
//////////////////////////////////////
void setLoRaMode()
{
  Serial.println("Setting LoRa Mode");
  setMode(RF92_MODE_SLEEP);
  writeRegister(REG_OPMODE,0x80);
   
  Serial.println("LoRa Mode Set");
  return;
}