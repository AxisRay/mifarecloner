// This example writes a MIFARE memory block 0x08. It is tested with a new MIFARE 1K cards. Uses default keys.
// Note: Memory block 0 is readonly and contains manufacturer data. Do not write to Sector Trailer block
// unless you know what you are doing. Otherwise, the MIFARE card may be unusable in the future.

//Contributed by Seeed Technology Inc (www.seeedstudio.com)

#include <PN532.h>
#include <SPI.h>

/*Chip select pin can be connected to D10 or D9 which is hareware optional*/
/*if you the version of NFC Shield from SeeedStudio is v2.0.*/
#define PN532_CS 10
PN532 nfc(PN532_CS);

uint8_t written=0;
#define  NFC_DEMO_DEBUG 1

void setup(void) {
#ifdef NFC_DEMO_DEBUG
  Serial.begin(9600);
  Serial.println("Hello!");
#endif
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
#ifdef NFC_DEMO_DEBUG
    Serial.print("Didn't find PN53x board");
#endif
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
  // configure board to read RFID tags and cards
  nfc.SAMConfig();
}

void loop(void) 
{
  uint32_t id;
  // look for MiFare type cards
  id = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);

  if (id != 0) 
  {
#ifdef NFC_DEMO_DEBUG
    Serial.print("Read card #"); 
    Serial.println(id);
    Serial.println();
#endif
    uint8_t keys[]= {
      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF    };
    uint8_t writeBuffer[16];
    for(uint8_t i = 0;i < 16;i ++)
    { 
      writeBuffer[i]=i; //Fill buffer with 0,1,2....F
    }
    if(nfc.authenticateBlock(1, id ,0x08,KEY_A,keys)) //authenticate block 0x08
    {
      //if authentication successful

      if(written == 0) //Not written
      {
        written = nfc.writeMemoryBlock(1,0x08,writeBuffer); // Write writeBuffer[] to block 0x08
        if(written)
#ifdef NFC_DEMO_DEBUG
          Serial.println("Write Successful");   
#endif
      }


      uint8_t block[16];
      //read memory block 0x08
      if(nfc.readMemoryBlock(1,0x08,block))
      {
#ifdef NFC_DEMO_DEBUG
        Serial.println("Read block 0x08:"); 
        //if read operation is successful
        for(uint8_t i=0;i<16;i++)
        {
          //print memory block
          Serial.print(block[i],HEX);
          Serial.print(" ");
        }
        Serial.println();
#endif
      }
    }
  }

  delay(500);
}


