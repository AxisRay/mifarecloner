#include <PN532.h>
#include <SPI.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>


/*Chip select pin can be connected to D10 or D9 which is hareware optional*/
#define MYDEBUG 1
#define PN532_CS 10
PN532 nfc(PN532_CS);
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// default key
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

uint8_t KeyA_Seq[16];
uint8_t KeyB_Seq[16];
uint8_t AclBytes[16][3];

bool GetKeySeq()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Getting KeySeq.");
  lcd.setCursor(0,1);

  uint32_t id;
  id = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);
  if(id == 0)
  {
    return false;
  }
  for(uint8_t sectorn=0; sectorn<16; sectorn++)
  {
    lcd.print("*");

    bool authA=false;
    bool authB=false;
    for(uint8_t key=0; key<KEY_LIST_LENGTH; key++)
    {

      uint8_t blockn=(sectorn+1)*4-1;

      if(!authA && nfc.authenticateBlock(1,id,blockn,KEY_A,keyList[key]))
      {
        KeyA_Seq[sectorn]=key;
        authA=true;

        uint8_t block[16];
        if(nfc.readMemoryBlock(1, blockn, block))
        {
          for(uint8_t k=0; k<3; k++){
              AclBytes[sectorn][k]=block[k+6];
          }
        }
        else
        {
          ErrorReport(blockn,"Read Fail!      ");
          return false;
        }

        #ifdef MYDEBUG
        for (uint8_t j = 0; j < 6; j++)
        {
          if(keyList[key][j]<=0x0F)
          {
            Serial.print("0");
          }
          Serial.print(keyList[key][j], HEX);
          Serial.print(" ");
        }
        Serial.print("| KEY_A | Sector ");
        Serial.println(sectorn);
        #endif
      }
      else
      {
        id = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);
        if(id == 0){
          Serial.println("NO TAG FOUND!");
          return false;
        } 
      }
      if(!authB && nfc.authenticateBlock(1,id,blockn,KEY_B,keyList[key]))
      {
        KeyB_Seq[blockn]=key;
        authB=true;

        #ifdef MYDEBUG
        for (uint8_t k = 0; k < 6; k++)
        {
          if(keyList[key][k]<=0x0F)
          {
            Serial.print("0");
          }
          Serial.print(keyList[key][k], HEX);
          Serial.print(" ");
        }
        Serial.print("| KEY_B | Sector ");
        Serial.println(sectorn);
        #endif
      }
      else
      {
        id = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);
        if(id == 0){
          Serial.println("NO TAG FOUND!");
          return false;
        }
      }
      if(authA && authB){
        Serial.println("ALL!");
        break;
      }
    }
    if(authA==false || authB==false) {
      lcd.clear();
      Serial.println("NO KEY FOUND!");
      return false;
    }
  }
  Serial.println("KeySeq Found!");
  
  return true;
}

#define  NFC_DEMO_DEBUG 1

uint32_t lastReadTag = 1;
uint32_t lastWriteTag = 1;
bool lastReadSuccess = false;
bool lastWriteSuccess = false;

void setup(void)
{

  #ifdef NFC_DEMO_DEBUG
  Serial.begin(9600);
  Serial.println("Hello!");
  #endif

  nfc.begin();
  lcd.begin(16, 2);

  lcd.setCursor(0, 0);
  lcd.print("NFC Tag Cloner  ");

  uint32_t versiondata = nfc.getFirmwareVersion();

  if (! versiondata)
  {

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
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);
  Serial.print("Supports ");
  Serial.println(versiondata & 0xFF, HEX);
  #endif

  lcd.setCursor(0, 1);
  lcd.print("FOUND CHIP PN53x");
  delay(1000);
  lcd.setCursor(0, 1);
  lcd.print("Firmware ver.1.0");
  delay(1000);
  // configure board to read RFID tags and cards
  nfc.SAMConfig();

}

void DebugEEPROM()
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
  WaitForReturn();
}

void WaitForReturn()
{
  Serial.println("Test!!!!!!!!!!");
  while (1) {
    if ( _read_buttons() == btnSELECT){ //SELECT
      delay(50);
      if ( _read_buttons() == btnNONE) {
        return;
      }
    }
  }
}

void ReturnToMenu(void)
{
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

// uint8_t KeyA_List[16][6];
// uint8_t KeyB_List[16][6];
// void ReadKeyFromEEPROM()
// {
//   for(int sector=0; sector<16; sector++){
//     blockn=(sector+1)*4-1
//     addr=blockn*16;
//     for(int i=0; i<6; i++){
//       KeyA_List[sector][i]=EEPROM.read(addr+i);
//       KeyB_List[sector][i]=EEPROM.read(addr+4+i);
//     }
//   }
//   #ifdef MYDEBUG
//   Serial.println();
//   for(int j=0; j<16; j++){
//     Serial.print("Sector ");
//     Serial.print(j);
//     Serial.print(" | KEY_A:");
//     for(int k=0; k<6; k++){
//       if(KeyA_List[i][k]<=0x0F)
//       {
//         Serial.print("0");
//       }
//       Serial.print(KeyA_List[i][k], HEX);
//       Serial.print(" ");
//     }
//     Serial.print(" | KEY_B:");
//     for(int k=0; k<6; k++){
//       if(KeyB_List[i][k]<=0x0F)
//       {
//         Serial.print("0");
//       }
//       Serial.print(KeyB_List[i][k], HEX);
//       Serial.print(" ");
//     }
//   }
//   #endif
// }
// bool ReadKeyFromTag()
// {
//   uint32_t id;
//   id = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);
//   if (id != 0)
//   {
//     Serial.println("find new tag");
//     #ifdef NFC_DEMO_DEBUG
//     Serial.print("Read card #");
//     Serial.println(id);
//     Serial.println();
//     #endif

//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Get KeyA&KeyB...");

//     //EEPROM Address
//     int addr = 0;

//     //ALL Blocks Except 1
//     for (uint8_t sector = 0; sector < 16; sector++)
//     {
//       uint8_t blockn=(sector+1)*4-1;
//       lcd.setCursor(sector, 1);
//       lcd.print("*");
//       if(nfc.authenticateBlock(1, id , blockn, KEY_A, keyList[KeyA_Seq[Sector]]))
//       {
//         uint8_t block[16];
//         if (nfc.readMemoryBlock(1, blockn, block))
//         {
//           //Each Bytes
//           for (uint8_t i = 0; i < 6; j++)
//           {
//             KeyA_List[sector][i]=block[i];
//             KeyB_List[sector][i]=block[i+4);
//           }
//         }
//         else{
//           ErrorReport(blockn,"Read Fail!      ");
//           return false;
//         }
//       }
//       else{
//         ErrorReport(blockn,"Auth Fail!      ");
//         return false;
//       }
//     }

//     #ifdef MYDEBUG
//     Serial.println();
//     for(uint8_t j=0; j<16; j++){
//       Serial.print("Sector ");
//       Serial.print(j);
//       Serial.print(" | KEY_A:");
//       for(uint8_t k=0; k<6; k++){
//         if(KeyA_List[i][k]<=0x0F)
//         {
//           Serial.print("0");
//         }
//         Serial.print(KeyA_List[i][k], HEX);
//         Serial.print(" ");
//       }
//       Serial.print(" | KEY_B:");
//       for(uint8_t k=0; k<6; k++){
//         if(KeyB_List[i][k]<=0x0F)
//         {
//           Serial.print("0");
//         }
//         Serial.print(KeyB_List[i][k], HEX);
//         Serial.print(" ");
//       }
//     }
//     #endif

//     Serial.println("KEY_A and KEY_B have been saved!");
//     return true;
//   }
//   Serial.println("No Tag Found! ");
//   return false;
// }


void WriteToTag(void)
{
  uint32_t id;
  id = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);
  if (id != 0)
  {
    #ifdef NFC_DEMO_DEBUG
    Serial.print("Read card #");
    Serial.println(id);
    Serial.println();
    #endif

    //EEPROM Address
    int addr;
    int blockn_start;
    
    lcd.setCursor(0, 0);
    lcd.print("Write Block 0 ? ");
    lcd.setCursor(0, 1);
    lcd.print("L: Yes     R: No");
    //Select
    bool flag = true;
    while (flag) {
      switch (_read_buttons())
      {
        case btnRIGHT:
        if (_read_buttons() == btnNONE) {
          blockn_start = 1;
          addr = 16;
          flag = false;
        }
        break;
        case btnLEFT:
        if (_read_buttons() == btnNONE) {
          blockn_start = 0;
          addr = 0;
          flag = false;
        }
        break;
        case btnSELECT:
        if (_read_buttons() == btnNONE) {
          return;
        }
      }
    }
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Writing...      ");
    for (uint8_t blockn = blockn_start; blockn < 64; blockn++)
    {
      lcd.setCursor(blockn / 4, 1);
      lcd.print("*");

      bool auth = false;

      Serial.print("try to Write Block");
      Serial.println(blockn);

      //Try Default keys
      for (int i = 0; i < KEY_LIST_LENGTH; i++)
      {
        Serial.print("Try key: ");
        Serial.println(i);

        //Try Auth
        if (nfc.authenticateBlock(1, id , blockn,KEY_A, keyList[i]))
        {
          Serial.println("Auth success,try to Write");

          //Prepare Block Buffer
          uint8_t block[16];
          for (int j = 0; j < 16; j++) {
            block[j] = EEPROM.read(addr);
            addr++;
          }
          if (nfc.writeMemoryBlock(1, blockn, block))
          {

            Serial.println("write success,try another block");
            auth = true;
            break;
          }
          else
          {
            Serial.println("Write Fail");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("ERROR!  Block ");
            lcd.print(blockn, DEC);
            lcd.setCursor(0, 1);
            lcd.print("Write Fail! R?");
            WaitForReturn();
            return;
          }
        }
        Serial.println("Auth Fail,Try Another Key");
        id = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);
      }
      if (auth == false)
      {
        Serial.println("All keys have been tried!");
        //All keys have been tried!
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ERROR!  Block ");
        lcd.print(blockn, DEC);
        lcd.setCursor(0, 1);
        lcd.print("No Key Found! ");
        WaitForReturn();
        return;
      }
    }
    //all the blocks have been writed
    lastWriteSuccess = true;
    Serial.println("all the blocks have been writed");
    ReturnToMenu();
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("No Tag Found! ");
  delay(1500);
  return;
}


void ErrorReport(uint8_t blockn,char *msg)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ERROR!  Block ");
  lcd.print(blockn, DEC);
  lcd.setCursor(0, 1);
  lcd.print(msg);
  WaitForReturn();
  return;
}

void ReadAndSave(void)
{
  // look for MiFare type cards
  uint32_t id;
  id = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);
  if (id != 0)
  {
    Serial.println("find new tag");
    #ifdef NFC_DEMO_DEBUG
    Serial.print("Read card #");
    Serial.println(id);
    Serial.println();
    #endif

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Reading...      ");

    //EEPROM Address
    int addr = 0;

    //ALL Blocks Except 1
    for (uint8_t blockn = 0; blockn < 64; blockn++)
    {
      lcd.setCursor(blockn/4, 1);
      lcd.print("*");
      
      Serial.print("Try to Read Block ");
      Serial.println(blockn);

      if(nfc.authenticateBlock(1, id , blockn, KEY_A, keyList[KeyA_Seq[blockn/4]]))
      {
        uint8_t block[16];
        if (nfc.readMemoryBlock(1, blockn, block))
        {
          //Each Bytes
          for (int j = 0; j < 16; j++)
          {
            //Write to EEPROM
            EEPROM.write(addr, block[j]);
            addr++;
          }

          #ifdef NFC_DEMO_DEBUG
            //if read operation is successful
            for (uint8_t i = 0; i < 16; i++)
            {
              if (block[i] <= 0xF) Serial.print("0");
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
            }
            
            else if (((blockn + 1) % 4) == 0){
              Serial.println("Sector Trailer");
            }

            else{
              Serial.println("Data Block");
            }
            #endif
          }
          else
          {
            ErrorReport(blockn,"Read Fail!      ");
            return;
          }
        }
        else
        {
          ErrorReport(blockn,"Auth Fail!      ");
          return;
        }
      }
      Serial.println("All the blocks have been saved");
      ReturnToMenu();
      return;
    }
    else
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("No Tag Found! ");
      delay(1500);
    }
  }

  void loop(void)
  {
  //Serial.println("Test1");
  lcd.setCursor(0, 0);
  lcd.print("UP:  Read & Save");
  lcd.setCursor(0, 1);
  lcd.print("DOWN: Write Tag ");
  switch (_read_buttons())
  {
    case btnUP:
    delay(50);
    if (_read_buttons() == btnNONE) {
      Serial.println("key up pressed");
      //if(GetReadKeySeq()){
        //ReadAndSave();
      //}
      GetKeySeq();
    }
    break;
    case btnDOWN:
    delay(50);
    if (_read_buttons() == btnNONE) {
      Serial.println("key down pressed");
      WriteToTag();
        //DebugEEPROM();
      }
      break;
      case btnRIGHT:
      delay(50);
      if (_read_buttons() == btnNONE) {
        Serial.println("key right pressed");
        //WriteToTag();
        //DebugEEPROM();
        if(GetKeySeq()){
          Serial.println("succeed");
        }
        Serial.println("Fail");
        lcd.clear();
      }
      break;
    }
  }
