#include <PN532.h>
#include <SPI.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
/*Chip select pin can be connected to D10 or D9 which is hareware optional*/
/*if you the version of NFC Shield from SeeedStudio is v2.0.*/
#define PN532_CS 10
PN532 nfc(PN532_CS);
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#define  NFC_DEMO_DEBUG 1

#define KEY_LIST_LENGTH 5
static uint8_t keyList[KEY_LIST_LENGTH][6] = {
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0x01, 0x33, 0x03, 0x03, 0x23, 0x23},
  {0x0A, 0xCB, 0xC8, 0x5D, 0x55, 0x81},
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
  {0xA9, 0xDE, 0x7F, 0x3C, 0xEB, 0x1F}
};

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE  -1

int _read_buttons()
{
  int adc_key_in = analogRead(0);
  if (adc_key_in < 50)   return btnRIGHT;
  if (adc_key_in < 195)  return btnUP;
  if (adc_key_in < 380)  return btnDOWN;
  if (adc_key_in < 555)  return btnLEFT;
  if (adc_key_in < 790)  return btnSELECT;
  return btnNONE;
}
void WaitForReturn(){
  while (1) {
    if ( _read_buttons() == btnSELECT){ //SELECT
      delay(80);
      if ( _read_buttons() == btnNONE) {
        return;
      }
    }
  }
}
void ReturnToMenu(void){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Finished!!!     ");
  lcd.setCursor(0, 1);
  lcd.print("Return in   sec.");
  for (int i = 3; i >= 0;  i--){
    lcd.setCursor(10, 1);
    lcd.print(i, DEC);
    delay(1000);
  }
  return;
}
void ErrorReport(int blockn,char *msg){
  lcd.clear();
  lcd.setCursor(0, 0);
  if(blockn<0)
  {
    lcd.print("ERROR!");
    }else{
      lcd.print("ERROR!  Block ");
      lcd.print(blockn, DEC);
    }
    lcd.setCursor(0, 1);
    lcd.print(msg);
    WaitForReturn();
    return;
  }

  bool ReadRegister(uint8_t* reg,uint8_t* result,uint8_t len){
    uint8_t cmd[1+2*len];
  cmd[0]=0x06;//ReadRegister
  for(uint8_t i=0; i<2*len; i++){
    cmd[i+1]=reg[i];
  }
  if(nfc.sendCommandCheckAck(cmd,1+2*len)){
    nfc.read(result,6+len+2);
    return true;
    }else{
      return false;
    }
  }
  bool WriteRegister(uint8_t* reg,uint8_t len){
    uint8_t cmd[1+3*len];
    uint8_t result[6+0+2];
  cmd[0]=0x08;//WriteRegister
  for(uint8_t i=0; i<3*len; i++){
    cmd[i+1]=reg[i];
  }
  if(nfc.sendCommandCheckAck(cmd,1+3*len)){
    nfc.read(result,6+0+2);
    return true;
    }else{
      return false;
    }
  }
  bool InCommunicateThru(uint8_t* data,uint8_t len){
    uint8_t cmd[1+len];
  cmd[0]=0x42;//InCommunicateThru
  for(uint8_t i=0; i<len; i++){
    cmd[i+1]=data[i];
  }
  if(nfc.sendCommandCheckAck(cmd,1+len)){

    return true;
    }else{
      return false;
    }
  }
  bool Unlock(){
  //HALT
  uint8_t regState1[6]={0x63, 0x02, 0x00, 0x63, 0x03, 0x00};
  if(!WriteRegister(regState1,6/3)){
    return false;
  }
  uint8_t halt[4]={0x50, 0x00, 0x57, 0xcd};
  if(!InCommunicateThru(halt,4)){
    return false;
  }
  //UNLOCK1
  uint8_t reg1[3]={0x63, 0x3d, 0x07};
  if(!WriteRegister(reg1,3/3)){
    return false;
  }
  uint8_t unlock1[1]={0x40};
  if(!InCommunicateThru(unlock1,1)){
    return false;
  }
  //UNLOCK2
  uint8_t reg2[3]={0x63, 0x3d, 0x00};
  if(!WriteRegister(reg2,3/3)){
    return false;
  }
  uint8_t unlock2[1]={0x43};
  if(!InCommunicateThru(unlock2,1)){
    return false;
  }
  uint8_t regState2[6]={0x63, 0x02, 0x80, 0x63, 0x03, 0x80};
  if(!WriteRegister(regState2,6/3)){
    return false;
  }
  return true;
}

void ShowEEPROM()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("EEPROM Checking!");
  
  int addr = 0;
  for (uint8_t blockn = 0; blockn < 64; blockn++)
  {
    lcd.setCursor(blockn / 4, 1);
    lcd.print("*");

    uint8_t block[16];
    for (uint8_t j = 0; j < 16; j++) {
      block[j] = EEPROM.read(addr);
      addr++;
    }
    #ifdef NFC_DEMO_DEBUG
    //if read operation is successful
    for (uint8_t i = 0; i < 16; i++)
    {
      //print memory block

      if (block[i] <= 0xF) //Data arrangement / beautify
      {
        Serial.print("0");
      }
      Serial.print(block[i], HEX);
      Serial.print(" ");
    }

    Serial.print("| Block ");
    if (blockn <= 9) //Data arrangement / beautify
    {
      Serial.print(" ");
    }
    Serial.print(blockn, DEC);
    Serial.print(" | ");

    if (blockn == 0)
    {
      Serial.println("Manufacturer Block");
    }
    else
    {
      if (((blockn + 1) % 4) == 0)
      {
        Serial.println("Sector Trailer");
      }
      else
      {
        Serial.println("Data Block");
      }
    }
    #endif
  }
  ReturnToMenu();
  return;
}

void ForceRead(){
  uint32_t id;
  id = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);
  if(id==0){
    ErrorReport(-1,"No Mifare Card!!");
    return;
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Unlocking...    ");
  if(!Unlock()){
    ErrorReport(-1,"Unlock Failed!  ");
    return;
  }
  lcd.setCursor(0,0);
  lcd.print("Force Reading...");
  int addr=0;
  for(int blockn=0; blockn<64; blockn++){
    lcd.setCursor(blockn/4, 1);
    lcd.print("*");
    uint8_t block[16];
    if (nfc.readMemoryBlock(1, blockn, block)){
      //Each Bytes
      for (uint8_t j = 0; j < 16; j++){
        //Write to EEPROM
        EEPROM.write(addr, block[j]);
        addr++;
      }

      //DEBUG
      for (uint8_t i = 0; i < 16; i++){
        if (block[i] <= 0xF){
          Serial.print("0");
        }
        Serial.print(block[i], HEX);
        Serial.print(" ");
      }
      Serial.print("| Block ");

      if (blockn <= 9){
        Serial.print(" ");
      } //Data arrangement / beautify

      Serial.print(blockn, DEC);
      Serial.print(" | ");

      if (blockn == 0){
        Serial.println("Manufacturer Block");
      }else if (((blockn + 1) % 4) == 0){
        Serial.println("Sector Trailer");
      }else{
        Serial.println("Data Block");
      }
      //DEBUG
    }
    else{
      ErrorReport(blockn,"Read Fail!      ");
      return;
    }
  }
  ReturnToMenu();
  return;
}
void ForceWrite(){
  uint32_t id;
  id = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);
  if(id==0){
    ErrorReport(-1,"No Mifare Card!!");
    return;
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Unlocking...    ");
  if(!Unlock()){
    ErrorReport(-1,"Unlock Failed!  ");
    return;
  }
  lcd.setCursor(0,0);
  lcd.print("Force Writing...");
  int addr=0;
  for(int blockn=0; blockn<64; blockn++){
    lcd.setCursor(blockn/4, 1);
    lcd.print("*");
    uint8_t block[16];
    for (int j = 0; j < 16; j++) {
      block[j] = EEPROM.read(addr);
      addr++;
    }
    if (!nfc.writeMemoryBlock(1, blockn, block)){
      ErrorReport(blockn,"Write Fail! R?");
      return;
    }
  }
  ReturnToMenu();
  return;
}
void setup(void) {
  #ifdef NFC_DEMO_DEBUG
  Serial.begin(9600);
  Serial.println("Hello!");
  #endif
  nfc.begin();
  lcd.begin(16,2);

  lcd.setCursor(0, 0);
  lcd.print("NFC Tag Cloner  ");

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    #ifdef NFC_DEMO_DEBUG
    Serial.print("Didn't find PN53x board");
    #endif
    lcd.setCursor(0, 1);
    lcd.print("PN53x NOT Found!");
    while (1); // halt
  }
  #ifdef NFC_DEMO_DEBUG
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); 
  Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); 
  Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); 
  Serial.println((versiondata>>8) & 0xFF, DEC);
  Serial.print("Supports "); 
  Serial.println(versiondata & 0xFF, HEX);
  #endif

  lcd.setCursor(0, 1);
  lcd.print("FOUND CHIP PN532");
  delay(1000);
  lcd.setCursor(0, 1);
  lcd.print("initializing....");
  delay(1000); 
  // configure board to read RFID tags and cards
  nfc.SAMConfig();
}

void loop(void) {
  // uint32_t id;
  // id = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);
  // nfc.authenticateBlock(1,id,0,KEY_A,keyList[0]);
  // uint8_t block[16];
  // if(nfc.readMemoryBlock(1, 0, block)){
  //   Serial.println("Success");
  // } else{
  //   Serial.println("fail");
  // }
  lcd.setCursor(0, 0);
  lcd.print("UP:  Force Read ");
  lcd.setCursor(0, 1);
  lcd.print("DN:  Force Write");
  while(1){
      switch (_read_buttons())
      {
        case btnUP:
          delay(80);
          if (_read_buttons() == btnNONE) {
            Serial.println("key up pressed");
            ForceRead();
            return;
          }
          break;
        case btnDOWN:
          delay(80);
          if (_read_buttons() == btnNONE) {
            Serial.println("key down pressed");
            ForceWrite();
            return;
          }
          break;
        case btnRIGHT://DEBUG
          delay(80);
          if (_read_buttons() == btnNONE) {
            Serial.println("key right pressed");
            ShowEEPROM();
            return;
          }
          break;
      }  // statement
  }

}




