#ifndef __NFC_P2P_H__
#define __NFC_P2P_H__

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


//#include <Arduino.h>
#include <SPI.h>
//#include <stdbool.h>
// this library is designed to be used with PN532 through the SPI interface only
// make sure that SPI operation is set at PN532 board (I0=0, I1=1), see UM0701-02 rev.02 p.24
// arduino is SPI master, PN532 is SPI slave
//Very important note: some issues are found to appear if no SAM configuration take place. 
//This error was pretty hard to find. With Arduino uno it happen not very often. With Leonardo always

//typedef bool boolean;

#define NFC_P2P_DEBUG 0

#define NFC_P2P_PN532_PREAMBLE 0x00
#define NFC_P2P_PN532_STARTCODE1 0x00
#define NFC_P2P_PN532_STARTCODE2 0xFF
#define NFC_P2P_PN532_POSTAMBLE 0x00

#define NFC_P2P_PN532_HOSTTOPN532 0xD4	// Used in Normal and extended information frames UM0701-02 p.28n
#define NFC_P2P_PN532_PN532TOHOST 0xD5	// Used in Normal and extended information frames UM0701-02 p.28n

// Miscellaneous commands
#define NFC_P2P_PN532_CMD_DIAGNOSE 0x00					// UM0701-02 p.69
#define NFC_P2P_PN532_CMD_GETFIRMWAREVERSION 0x02		// UM0701-02 p.73
#define NFC_P2P_PN532_CMD_GETGENERALSTATUS 0x04			// UM0701-02 p.74
#define NFC_P2P_PN532_CMD_READREGISTER 0x06				// UM0701-02 p.76
#define NFC_P2P_PN532_CMD_WRITEREGISTER 0x08			// UM0701-02 p.78
#define NFC_P2P_PN532_CMD_READGPIO 0x0C					// UM0701-02 p.79
#define NFC_P2P_PN532_CMD_WRITEGPIO 0x0E				// UM0701-02 p.81
#define NFC_P2P_PN532_CMD_SETSERIALBAUDRATE 0x10		// UM0701-02 p.83
#define NFC_P2P_PN532_CMD_SETPARAMETERS 0x12			// UM0701-02 p.85
#define NFC_P2P_PN532_CMD_SAMCONFIGURATION  0x14		// UM0701-02 p.89
#define NFC_P2P_PN532_CMD_POWERDOWN  0x16				// UM0701-02 p.98

// RF communication commands
#define NFC_P2P_PN532_CMD_RFCOMMUNICATION 0x32			// UM0701-02 p.101
#define NFC_P2P_PN532_CMD_RFREGULATIONTEST 0x58			// UM0701-02 p.107

// Initiator commands
#define NFC_P2P_PN532_CMD_INJUMPFORDEP 0x56				// UM0701-02 p.108
#define NFC_P2P_PN532_CMD_INJUMPFORPSL 0x46				// UM0701-02 p.113
#define NFC_P2P_PN532_CMD_INLISTPASSIVETARGET 0x4A		// UM0701-02 p.115
#define NFC_P2P_PN532_CMD_INATR 0x50					// UM0701-02 p.115
#define NFC_P2P_PN532_CMD_INPSL 0x4E					// UM0701-02 p.125
#define NFC_P2P_PN532_CMD_INDATAEXCHANGE 0x40			// UM0701-02 p.127
#define NFC_P2P_PN532_CMD_INCOMMUNICATETHRU 0x42		// UM0701-02 p.136
#define NFC_P2P_PN532_CMD_INDESELECT 0x44				// UM0701-02 p.139
#define NFC_P2P_PN532_CMD_INRELEASE 0x52				// UM0701-02 p.140
#define NFC_P2P_PN532_CMD_INSELECT 0x54					// UM0701-02 p.141
#define NFC_P2P_PN532_CMD_INAUTOPOLL 0x60				// UM0701-02 p.144

// Target commands
#define NFC_P2P_PN532_CMD_TGINITASTARGET 0x8C			// UM0701-02 p.151
#define NFC_P2P_PN532_CMD_TGSETGENERALBYTES 0x92		// UM0701-02 p.158
#define NFC_P2P_PN532_CMD_TGGETDATA 0x86				// UM0701-02 p.160
#define NFC_P2P_PN532_CMD_TGSETDATA 0x8E				// UM0701-02 p.164
#define NFC_P2P_PN532_CMD_TGSETMETADATA 0x94			// UM0701-02 p.166
#define NFC_P2P_PN532_CMD_TGGETINITIATORCOMMAND 0x88	// UM0701-02 p.168
#define NFC_P2P_PN532_CMD_TGRESPONSETOINITIATOR 0x90	// UM0701-02 p.170
#define NFC_P2P_PN532_CMD_TGGETTARGETSTATUS 0x8A		// UM0701-02 p.172

// unique frame type identifiers
typedef enum { NFC_P2P_PN532_FRAME_NORMAL, NFC_P2P_PN532_FRAME_EXTENDED, NFC_P2P_PN532_FRAME_ACK, NFC_P2P_PN532_FRAME_NACK, NFC_P2P_PN532_FRAME_ERROR, NFC_P2P_PN532_FRAME_INVALID} t_frame_types;
typedef enum { NFC_P2P_UPDATE_OK=0, NFC_P2P_UPDATE_INVALIDFRAME, NFC_P2P_UPDATE_UNEXPECTED, NFC_P2P_UPDATE_INTERNALERROR} t_update_result;


// SPI definitions
#define NFC_P2P_SPI_SPEEDMAXIMUM 4000000 // <= 5MHz UM0701-02 rev.2 p.45
#define NFC_P2P_SPI_DATAMODE SPI_MODE0 // UM0701-02 rev.2 p.25
#define NFC_P2P_SPI_DATAORDER LSBFIRST // UM0701-02 rev.2 p.25
#define NFC_P2P_SPI_STATUSREADING  0x02	// host instructs the pn532 to send its spi status byte to host
#define NFC_P2P_SPI_DATAWRITING 0x01		// host wants to write data to pn532
#define NFC_P2P_SPI_DATAREADING  0x03	// host instructs the pn532 to send data to host
#define NFC_P2P_SPI_RDY 0				// ready bit of the spi status byte, see UM0701-02 p.45

#define NFC_P2P_WAKEUPTIME 2			// in ms, how long the pn532 takes to wake up from power down mode when CSS is becoming low; see UM0701-02 rev.2 p.57
#define	NFC_P2P_DEFAULT_TIMEOUT 400		// in ms, how long a response will be waited for
//230
#define NFC_P2P_BUFFERSIZE 115

typedef void(*t_cbGetFirmwareVersion)(uint8_t, uint8_t, uint8_t, uint8_t);
typedef void(*t_cbGetGeneralStatus)(uint8_t, uint8_t, uint8_t, uint8_t*, uint8_t);
typedef void(*t_cbNone)(void); // to be used with WriteRegister, WriteGPIO, SetSerialBaudRate, SetParameters, SAMConfiguration, RFConfiguration
typedef void(*t_cbByte)(uint8_t); // to be used with PowerDown, InPSL, InDeselect, InRelease, InSelect, TgSetGeneralBytes, TgSetData, TgSetMetaData, TgResponseToInitiator
typedef void(*t_cbInJumpFor)(uint8_t, uint8_t, uint8_t*, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t*); // to be used with InJumpForDEP and InJumpForPSL; added 8-bit parameter for length of Gt
typedef void(*t_cbInListPassiveTarget)(uint8_t, uint8_t*);
typedef void(*t_cbInAtr)(uint8_t, uint8_t*, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t*); // added 8-bit parameter for length of Gt
typedef void(*t_cbReceive)(uint8_t, uint16_t, uint8_t*); // to be used with InDataExchange, InCommunicateThrough, TgGetData; added 16-bit parameter for length of DataIn (up to 262 bytes), see UM0701-02 rev.2 p.127 and p.28/29
typedef void(*t_cbInAutoPoll)(uint8_t, uint8_t*);
typedef void(*t_cbTgGetInit)(uint8_t, uint16_t, uint8_t*); // Mode, lenInitiatorCommand, InitiatorCommand[]; to be used with TgInitAsTarget, TgGetInitiatorCommand
typedef void(*t_cbTgGetTargetStatus)(uint8_t, uint8_t);
typedef void(*t_cbBytes)(uint8_t, uint8_t*); // number of bytes and pointer to bytes; to be used with ReadRegister
typedef void(*t_cbReadGpio)(uint8_t, uint8_t, uint8_t); // P3, P7, I0I1 UM0701-02 rev.2 p.79

class nfc_p2p {
public:
	nfc_p2p(uint8_t spiChipSelect); // constructor; spiChipSelect: chip select pin that goes to the NSS pin of the PN532 (active low) 
	//nfc_p2p(uint8_t spiChipSelect, t_cbByte errorHandler);
	uint8_t update(void);

	// PN532 commands
	boolean GetFirmwareVersion(t_cbGetFirmwareVersion cb);
	boolean GetGeneralStatus(t_cbGetGeneralStatus cb);
	boolean ReadRegister(uint8_t numberRegisters, uint16_t* adresses, t_cbBytes cb);
	boolean WriteRegister(uint8_t numberRegisters, uint16_t* adresses, uint8_t values, t_cbNone);
	boolean ReadGPIO(t_cbReadGpio cb);
	boolean WriteGPIO(uint8_t p3, uint8_t p7, t_cbNone cb);
	boolean SetSerialBaudRate(uint8_t baudRate, t_cbNone cb); // see UM0701-02 rev.2 p.83
	boolean SetParameters(uint8_t flags, t_cbNone cb); // see UM0701-02 rev.2 p.85
	boolean SAMConfiguration(uint8_t mode, uint8_t timeout, uint8_t irq, t_cbNone cb); // see UM0701-02 rev.2 p.89
	boolean PowerDown(uint8_t wakeupEnable, uint8_t generateIrq, t_cbByte cb); // see UM0701-02 rev.2 p.98
	boolean RFConfiguration(uint8_t configurationItem, uint8_t* configurationData, t_cbNone cb); // see UM0701-02 rev.2 p.101
	boolean InJumpForDEP(boolean activeMode, uint8_t baudRate, uint8_t next, uint8_t lenPassiveInitiatorData, uint8_t* passiveInitiatorData, uint8_t lenNfcId3i, uint8_t* nfcId3i, uint8_t lenGeneralBytes, uint8_t* generalBytes, t_cbInJumpFor cb); // see UM0701-02 rev.2 p. 108
	boolean InJumpForPSL(boolean activeMode, uint8_t baudRate, uint8_t lenPassiveInitiatorData, uint8_t* passiveInitiatorData, uint8_t lenNfcId3i, uint8_t* nfcId3i, uint8_t lenGeneralBytes, uint8_t* generalBytes, t_cbInJumpFor cb); // see UM0701-02 rev.2 p. 113
	boolean InListPassiveTarget(uint8_t maxNumberOfTargets, uint8_t baudRateInit, uint8_t lenInitiatorData, uint8_t* initiatorData, t_cbInListPassiveTarget cb);
	boolean InAtr(uint8_t relevantTarget, uint8_t lenNfcId3i, uint8_t* nfcId3i, uint8_t lenGi, uint8_t* Gi, t_cbInAtr cb); // see UM0701-02 rev.2 p.122
	boolean InPSL(uint8_t relevantTarget, uint8_t baudRateIn2Tg, uint8_t baudRateTg2In, t_cbByte cb);
	boolean InDataExchange(uint8_t relevantTarget, uint16_t lenDataOut, uint8_t* dataOut, t_cbReceive cb); // see UM0701-02 rev.2 p.127
	boolean InCommunicateThrough(uint16_t lenDataOut, uint8_t* dataOut, t_cbReceive cb); // see UM0701-02 rev.2 p.136
	boolean InDeselect(uint8_t relevantTarget, t_cbByte cb);
	boolean InRelease(uint8_t relevantTarget, t_cbByte cb);
	boolean InSelect(uint8_t relevantTarget, t_cbByte cb);
	boolean InAutoPoll(uint8_t pollingNumber, uint8_t period, uint8_t lenTargetTypes, uint8_t* targetTypes, t_cbInAutoPoll cb); // period in multiples of 150ms, at least 1 target type must be specified; see UM0701-02 rev.2 p.144
	boolean TgInitAsTarget(uint8_t mode, uint8_t mifareParams[6], uint8_t felicaParams[18], uint8_t nfcId3t[10], uint8_t lenGeneralBytes, uint8_t* generalBytes, uint8_t lenHistoricalBytes, uint8_t* historicalBytes, t_cbTgGetInit cb);
	boolean TgSetGeneralBytes(uint8_t lenGeneralBytes, uint8_t* generalBytes, t_cbByte cb);
	boolean TgGetData(t_cbReceive cb);
	boolean TgSetData(uint16_t lenDataOut, uint8_t* dataOut, t_cbByte cb); // to be used when lenDataOut<=262
	boolean TgSetMetaData(uint16_t lenDataOut, uint8_t* dataOut, t_cbByte cb); // to be used when if lenDataOut>262
	boolean TgGetInitiatorCommand(t_cbTgGetInit cb);
	boolean TgResponseToInitiator(uint16_t lenResponse, uint8_t* response, t_cbByte cb);
	boolean TgGetTargetStatus(t_cbTgGetTargetStatus cb);
	boolean abortPreviousCommand(void);
private:
uint8_t spiChipSelect;
	//SPIClass SPI;
	
	uint8_t transmitBuffer[NFC_P2P_BUFFERSIZE];
	uint16_t lenTransmitBuffer;
	uint8_t receiveBuffer[NFC_P2P_BUFFERSIZE];
	uint16_t lenReceiveBuffer;
	uint8_t lastCommand=0;
	boolean busy=false;

	void spiBegin(void);
	void spiEnd(void);
	uint8_t spiStatusReading(void);
	t_frame_types spiDataReading(uint8_t timeout);
	void spiDataWriting(uint16_t len, uint8_t *dataOut);
	void spiWriteByte(uint8_t dataOut);
	uint8_t spiReadByte(void);
	boolean hasData(void);

	// callbacks
	t_cbGetFirmwareVersion cbGetFirmwareVersion;
	t_cbGetGeneralStatus cbGetGeneralStatus;
	t_cbNone cbNone;
	t_cbByte cbByte;
	t_cbInJumpFor cbInJumpFor;
	t_cbInListPassiveTarget cbInListPassiveTarget;
	t_cbInAtr cbInAtr;
	t_cbReceive cbReceive;
	t_cbInAutoPoll cbInAutoPoll;
	t_cbTgGetInit cbTgGetInit;
	t_cbTgGetTargetStatus cbTgGetTargetStatus;
	t_cbBytes cbBytes;
	t_cbReadGpio cbReadGpio;

	// error handler
	t_cbByte ehByte;

	boolean validateReceivedInformation(uint16_t len, uint8_t* buffer);
	boolean compareBytes(uint16_t len, uint8_t *P1, uint8_t *P2);
	boolean executeCommand(uint8_t cmd);
	void resetAllCallbacks();
	void assembleNormalInformationFrame(uint8_t len, uint8_t* dataOut);
	void assembleExtendedInformationFrame(uint16_t len, uint8_t* dataOut);
	void assembleInformationFrame(uint16_t len, uint8_t* dataOut);
	uint8_t calculateChecksum(uint8_t tfi, uint16_t len, uint8_t* data);	
};

#endif

