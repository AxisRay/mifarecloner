#include <PN532.h>
#include <SPI.h>

/*Chip select pin can be connected to D10 or D9 which is hareware optional*/
/*if you the version of NFC Shield from SeeedStudio is v2.0.*/
#define PN532_CS 10
PN532 nfc(PN532_CS);

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

char DataOut[]="HELLO TARGET!!!"; //16bytes
char DataIn[16];//Should be 16bytes
void loop(void) {

  // Configure PN532 as Peer to Peer Initiator in active mode
  if( nfc.configurePeerAsInitiator(PN532_BAUDRATE_424K) ) //if connection is error-free
  {
    //trans-receive data
    if(nfc.initiatorTxRx(DataOut,DataIn))
    {
#ifdef NFC_DEMO_DEBUG
      Serial.print("Data Sent and Received: ");
      Serial.println(DataIn);
#endif
    }
  }
}



