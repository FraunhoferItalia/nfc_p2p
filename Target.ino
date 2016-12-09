#include <nfc_p2p.h>
#include <SPI.h>
#define MY_LUNA_ADDRESS 0x03
#define PAD_TO_LOOK 0x03


#define PN532_CS A2
nfc_p2p nfc(PN532_CS);

#define  NFC_DEMO_DEBUG 1
void cbFirmwareVersion(uint8_t ic, uint8_t ver, uint8_t rev, uint8_t support) {
  #ifdef NFC_DEMO_DEBUG
    Serial.print("Firmware version IC=");
    Serial.print(ic, HEX);
    Serial.print(", Ver=");
    Serial.print(ver, HEX);
    Serial.print(", Rev=");
    Serial.print(rev, HEX);
    Serial.print(", Support=");
    Serial.println(support, HEX);
  #endif
}
uint8_t ATR_REQ[]={0x00,0xFF,0xFF,0x00,0x00};
uint8_t MIFARE[]={0x08, 0x00,0x12, 0x34, 0x56,0x40};
uint8_t FELICA[]={0x01, 0xFE, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,0xFF, 0xFF};
uint8_t NFCID3t[]={0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
//uint8_t HIST[]={0x00};
//uint8_t GEN[]={0x00};

uint8_t GEN[]={0x03};// INdirizzo della macchinina
int stateMachine = 0;


uint8_t dataOut[]="TG012346789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"; 


void cbSamConfig(void){
  Serial.println("Sam config executed");  
}

void cbTgInitialization(uint8_t mode, uint16_t len, uint8_t* InitiatorCommand){
  Serial.println("Target initialization executed");  

  Serial.println(mode,HEX);
  Serial.println(len,HEX);
  Serial.println(InitiatorCommand[8],HEX);
  
  stateMachine = 2;
}


void cbTgRx(uint8_t statusCommand, uint16_t len, uint8_t* dataIn){
    
  if(statusCommand!=0x00){
          Serial.println(F("error in cb cbTgRx"));
  }
      else
      {
      Serial.println("Target rx"); 

      /* for(uint8_t i =0;i<len;i++){
      Serial.println(dataIn[i],HEX);
       }*/
      stateMachine = 4; 
      }
}

void cbTgTx(uint8_t statusCommand){
   
  if(statusCommand!=0x00){
          Serial.println(F("error in cb cbTgTx"));
  }
      else
         {
      Serial.println("Target tx");  
      stateMachine = 6; 
      }
}

void setup(void) {

    Serial.begin(115200);
    while(!Serial);
    Serial.println("Hello, I am target!");
    nfc = nfc_p2p(PN532_CS);
    nfc.GetFirmwareVersion(&cbFirmwareVersion);
    nfc.update();
  
    Serial.println("call sam!");
    nfc.SAMConfiguration(0x01,0x14,0x01,&cbSamConfig);
    nfc.update();
  
}

void loop(void) {
  if(stateMachine == 0){
    uint8_t mylunaaddress = MY_LUNA_ADDRESS;
     nfc.TgInitAsTarget(0x00, MIFARE, FELICA, NFCID3t,1, &mylunaaddress, 0, NULL, &cbTgInitialization); // 0,Null,0,Null....here general byte
     stateMachine=1;
     
    
    }

    if(stateMachine == 2){
     
       Serial.println("Tring get in ino");
     nfc.TgGetData(&cbTgRx);
     stateMachine=3;
    }

 if(stateMachine == 4){
  // Serial.println(stateMachine);
     Serial.println("Tring set data ");
     nfc.TgSetData(sizeof(dataOut)/sizeof(dataOut[0]),dataOut, &cbTgTx);
     stateMachine=5;
   
    }

 
  nfc.update();
// delay(3000);
}
