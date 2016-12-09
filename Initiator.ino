#include <nfc_p2p.h>
#include <SPI.h>
#define MY_LUNA_ADDRESS 0x01
#define VEHICLE_TO_LOOK 0x03
#define PN532_CS 10
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
uint8_t RFConfigData[]={0x00,0x0A,0x09};
uint8_t flag = 0;
uint8_t GEN[]={0x01};// INdirizzo del PAD
uint8_t dataOut[]="IN012346789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"; 


void cbSamConfig(void){
  Serial.println("Sam config executed");  
}

void cbInitAsInitiator(uint8_t statusCommand, uint8_t targetNumber, uint8_t* nfcid3t, uint8_t did, uint8_t bst, uint8_t brt, uint8_t to, uint8_t ppt, uint8_t lenGt, uint8_t* Gt)
{
  Serial.println(F("inside callback"));
  if(statusCommand!=0x00){
   
  Serial.println(F("error in cb init as initiator in status"));
  }
  else{
  Serial.println(F("cb init as initiator inizializzator"));

           Serial.println(*nfcid3t,HEX);
           Serial.println(did,HEX);
           Serial.println(bst,HEX);
            Serial.println(brt,HEX);
             Serial.println(to,HEX);
              Serial.println(ppt,HEX);
               Serial.println(lenGt,HEX);
                Serial.println(*Gt++,HEX);
              
   flag=2;
  }
}


void cbInitiatorRx(uint8_t statusCommand, uint16_t len, uint8_t* dataIn){    
      if(statusCommand!=0x00){
          Serial.println(F("error in cb init as initiator in status"));
      }
      else{
          Serial.println(F("Initiator, data received from target"));
          
           for(uint8_t i =0;i<len;i++){
           Serial.println(dataIn[i],HEX);
           }
          flag=4;
      }
          
}


void cbRF(void){
  Serial.println("RF config executed");  
}

void setup(void) {

    Serial.begin(115200);
    Serial.println("Hello, I am initiator!");
    nfc = nfc_p2p(PN532_CS);
    nfc.GetFirmwareVersion(&cbFirmwareVersion);
    nfc.update();
  
    Serial.println("call sam!"); //sam could not be used in normal mode in dep,so our case
    nfc.SAMConfiguration(0x01,0x14,0x01,&cbSamConfig);
    nfc.update();

    nfc.RFConfiguration(0x02, RFConfigData, cbRF);
    nfc.update();
}

void loop(void) {
  if(flag==0){
  nfc.InJumpForDEP(0x01,0x02,0x02, 0,NULL, 5,  ATR_REQ, 1,  GEN, &cbInitAsInitiator);
  Serial.println("Injumpfordep called"); 
  flag=1;
  }

 if(flag==2){
  nfc.InDataExchange(0x01, sizeof(dataOut)/sizeof(dataOut[0]),dataOut,&cbInitiatorRx);
  Serial.println("Indataexchange called!");
  flag=3;
   }
  nfc.update();
 //delay(3000);
}


