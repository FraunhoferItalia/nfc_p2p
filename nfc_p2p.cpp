#include "nfc_p2p.h"

nfc_p2p::nfc_p2p(uint8_t spiChipSelect)
{
	pinMode(spiChipSelect, OUTPUT); // configure SS
	digitalWrite(spiChipSelect, HIGH); // active low
	SPI.begin(); // Initializes the SPI bus by setting SCK, MOSI, and SS to outputs, pulling SCK and MOSI low, and SS high.
	nfc_p2p::spiChipSelect = spiChipSelect; // init member function
	SPI.setDataMode(NFC_P2P_SPI_DATAMODE);
	SPI.setBitOrder(NFC_P2P_SPI_DATAORDER);
#if defined __SAM3X8E__
	SPI.setClockDivider(42);
#elif defined __SAMD21G18A__
	SPI.setClockDivider(24);
#else
	SPI.setClockDivider(NFC_P2P_SPI_SPEEDMAXIMUM);
#endif
}

boolean nfc_p2p::hasData(void) {
	if (spiStatusReading() != NFC_P2P_SPI_RDY)
		return true;
	else
		return false;
}

void nfc_p2p::spiBegin(void) {
	digitalWrite(spiChipSelect, LOW);
	delay(NFC_P2P_WAKEUPTIME);
}

void nfc_p2p::spiEnd(void) {
	SPI.endTransaction();
	digitalWrite(spiChipSelect, HIGH);
}

void nfc_p2p::spiWriteByte(uint8_t dataOut) {
	SPI.transfer(dataOut);
}

uint8_t nfc_p2p::spiReadByte(void) {
	uint8_t dataIn = SPI.transfer(0);
	return dataIn;
}

uint8_t nfc_p2p::spiStatusReading(void) {
	spiBegin();
	spiWriteByte(NFC_P2P_SPI_STATUSREADING);
	uint8_t status = spiReadByte();
	spiEnd();
	return status;
}

boolean nfc_p2p::compareBytes(uint16_t len,uint8_t *P1,uint8_t *P2 ) {
	uint16_t i;
	for (i = 0; i < len; i++) {
		if (P1[i] != P2[i]) {
			return false;
		}		
	}
	return true;

}

t_frame_types nfc_p2p::spiDataReading(uint8_t timeout) {
	uint16_t i;
	unsigned long starttime;
	uint8_t preamble[] = { NFC_P2P_PN532_PREAMBLE, NFC_P2P_PN532_STARTCODE1, NFC_P2P_PN532_STARTCODE2 };
	t_frame_types frameType;
	uint16_t len = 0;
	uint8_t dcs = 0;
	boolean over = false;

	spiBegin();
	spiWriteByte(NFC_P2P_SPI_DATAREADING);
	memset(receiveBuffer, 0, sizeof(receiveBuffer));
	lenReceiveBuffer = 0;
	// find preamble and start bytes
	for (i = 0; i < 3; i++) {
		receiveBuffer[i] = spiReadByte();
	}

	starttime = micros();
	while ((!compareBytes(3, receiveBuffer, preamble)) && (!over)) {
		// keep trying
		receiveBuffer[0] = receiveBuffer[1];
		receiveBuffer[1] = receiveBuffer[2];
		receiveBuffer[2] = spiReadByte();
		over = ((micros() - starttime) / 1000) < timeout;
	}

	// check for timeout
	if (over) {
		frameType =  NFC_P2P_PN532_FRAME_INVALID;
		//Serial.println(F("[nfc_p2p::spiDataReading] timeout"));
	}
	else {
		// keep reading other 3 bytes (min size of frames is 6 bytes)
		for (i = 3; i < 6; i++) {
			receiveBuffer[i] = spiReadByte();
		}

		// determine frame type and frame length
		switch (receiveBuffer[3]) {
		case 0x00:
			if ((receiveBuffer[4] == 0xff) && (receiveBuffer[5] == 0x00)) {
				frameType = NFC_P2P_PN532_FRAME_ACK;
				//Serial.println(F("[nfc_p2p::spiDataReading] valid ACK frame found"));
			}
			else {
				// error. this is not supposed to happen
				frameType = NFC_P2P_PN532_FRAME_INVALID;
				//Serial.println(F("[nfc_p2p::spiDataReading] invalid frame found"));
			}
			break;
		case 0x01:
			if (receiveBuffer[4] == 0xff) {
				frameType = NFC_P2P_PN532_FRAME_ERROR;
				if (receiveBuffer[3] + receiveBuffer[4]) {
					// error, length checksum not correct
					frameType = NFC_P2P_PN532_FRAME_INVALID;
					//Serial.println(F("[nfc_p2p::spiDataReading] length checksum not correct"));
					break;
				}
				len = 1;
				// read last 2 bytes
				for (i = 6; i < 8; i++) {
					receiveBuffer[i] = spiReadByte();
				}
				//Serial.println(F("[nfc_p2p::spiDataReading] valid ERROR frame found"));
			}
			else {
				// error. this is not supposed to happen
				frameType = NFC_P2P_PN532_FRAME_INVALID;
				//Serial.println(F("[nfc_p2p::spiDataReading] error: it's not a ERROR frame, nor something else valid"));
			}
			break;
		case 0xff:
			switch (receiveBuffer[4]) {
			case 0x00:
				if (receiveBuffer[5] == 0x00) {
					frameType = NFC_P2P_PN532_FRAME_NACK;
					//Serial.println(F("[nfc_p2p::spiDataReading] valid NACK frame found"));
				}
				else {
					// error. this is not supposed to happen
					frameType = NFC_P2P_PN532_FRAME_INVALID;
					//Serial.println(F("[nfc_p2p::spiDataReading] error: it's not a NACK frame, nor something else valid"));
				}
				break;
			case 0xff:
				// extended information frame
				frameType = NFC_P2P_PN532_FRAME_EXTENDED;
				// read 3 more bytes
				for (i = 6; i < 9; i++) {
					receiveBuffer[i] = spiReadByte();
				}
				if ((receiveBuffer[5] + receiveBuffer[6] + receiveBuffer[7]) & 0xff) {
					// error, length checksum not correct
					frameType = NFC_P2P_PN532_FRAME_INVALID;
					//Serial.println(F("[nfc_p2p::spiDataReading] length checksum not correct"));
					break;
				}
				len = receiveBuffer[5] << 8 + receiveBuffer[6];
				if (len > 265) {
					// error, length exceeds the firmware capabilities; see UM0701-02 rev.2 p.29
					frameType = NFC_P2P_PN532_FRAME_INVALID;
					//Serial.println(F("[nfc_p2p::spiDataReading] error: length more than firmware is capable to deliver"));
					break;
				}
				// read remaining bytes (including postamble)
				for (i = 9; i < 10 + len; i++) {
					receiveBuffer[i] = spiReadByte();
				}
				//Serial.println("valid EXTENDED frame found");
				break;
			default:
				// error. this is not supposed to happen
				frameType = NFC_P2P_PN532_FRAME_INVALID;
				//Serial.println(F("[nfc_p2p::spiDataReading] error: it's not a NACK frame, nor a EXTENDED frame, nor something else valid"));
			}

			break;
		default:
			// normal information frame
			frameType = NFC_P2P_PN532_FRAME_NORMAL;
			if ((receiveBuffer[3] + receiveBuffer[4]) & 0xff) {
				// error, length checksum not correct
				frameType = NFC_P2P_PN532_FRAME_INVALID;
				//Serial.println(F("[nfc_p2p::spiDataReading] length checksum not correct"));
				break;
			}
			len = receiveBuffer[3];
			// read remaining bytes (including postamble)
			for (i = 6; i < 7 + len; i++) {
				receiveBuffer[i] = spiReadByte();
			}
			//Serial.println(F("[nfc_p2p::spiDataReading] valid NORMAL frame found"));
		}
	}


	spiEnd();
	lenReceiveBuffer = i;
	busy = false;
	//Serial.println(F("[nfc_p2p::spiDataReading] busy->false"));

	// verify data checksum
	switch (frameType) {
	case NFC_P2P_PN532_FRAME_NORMAL:
		for (i = 5; i < lenReceiveBuffer - 1; i++) {
			dcs += receiveBuffer[i];
			//Serial.println(receiveBuffer[i], HEX);
		}
		break;
	case NFC_P2P_PN532_FRAME_EXTENDED:
		for (i = 8; i < lenReceiveBuffer - 1; i++) {
			dcs += receiveBuffer[i];
		}
		break;
	case NFC_P2P_PN532_FRAME_ERROR:
		for (i = 5; i < lenReceiveBuffer - 1; i++) {
			dcs += receiveBuffer[i];
		}
		break;
	}
	if (dcs & 0xff) {
		// data checksum error
		//Serial.println(F("[nfc_p2p::spiDataReading] data checksum error"));
		return NFC_P2P_PN532_FRAME_INVALID;
	}

	// parse receive buffer and evaluate contained information
	switch (frameType) {
	case NFC_P2P_PN532_FRAME_NORMAL:
		if (!validateReceivedInformation(len, receiveBuffer+5))
			return NFC_P2P_PN532_FRAME_INVALID;
		break;
	case NFC_P2P_PN532_FRAME_EXTENDED:
		if (!validateReceivedInformation(len, receiveBuffer+8))
			return NFC_P2P_PN532_FRAME_INVALID;
		break;
	case NFC_P2P_PN532_FRAME_ERROR:
		// error handler set?
		if (ehByte)
			// call error handler with specific application level error code
			ehByte(receiveBuffer[5]);
		break;
	case NFC_P2P_PN532_FRAME_ACK:
	case NFC_P2P_PN532_FRAME_NACK:
		;// do nothing
		break;
	}
	return frameType;
}

boolean nfc_p2p::validateReceivedInformation(uint16_t len, uint8_t* buffer) {
	// for normal and extended information frames
	// buffer shall point to TFI

	// verify information is coming from PN532
	if (buffer[0]!=NFC_P2P_PN532_PN532TOHOST) {
		//Serial.println(F("[nfc_p2p::validateReceivedInformation] frame not coming from PN532"));
		return false;
	}

	// verify received information is answer to last command
	uint8_t cmd = buffer[1];

	if (cmd!=lastCommand+1) {
		//Serial.println(F("[nfc_p2p::validateReceivedInformation] frame not answer to last command"));
		return false;
	}

	switch (cmd-1) {
	case NFC_P2P_PN532_CMD_DIAGNOSE:
		break;
	case NFC_P2P_PN532_CMD_GETFIRMWAREVERSION:
		if (cbGetFirmwareVersion==NULL) {
			//Serial.println(F("[nfc_p2p::validateReceivedInformation] callback not defined"));
			return false;
		}
		cbGetFirmwareVersion(buffer[2], buffer[3], buffer[4], buffer[5]);
		break;
	case NFC_P2P_PN532_CMD_GETGENERALSTATUS:
	break;
	case NFC_P2P_PN532_CMD_READREGISTER:
	break;
	case NFC_P2P_PN532_CMD_WRITEREGISTER:
	break;
	case NFC_P2P_PN532_CMD_READGPIO:
	break;
	case NFC_P2P_PN532_CMD_WRITEGPIO:
	break;
	case NFC_P2P_PN532_CMD_SETSERIALBAUDRATE:
	break;
	case NFC_P2P_PN532_CMD_SETPARAMETERS:
	break;
	case NFC_P2P_PN532_CMD_SAMCONFIGURATION:
		if (cbNone == NULL) {
			//Serial.println(F("[nfc_p2p::validateReceivedInformation] callback not defined"));
			return false;
		}
		cbNone();
		break;
	case NFC_P2P_PN532_CMD_POWERDOWN:
	break;
	case NFC_P2P_PN532_CMD_RFCOMMUNICATION:
		if (cbNone == NULL) {
			//Serial.println(F("[nfc_p2p::validateReceivedInformation] callback not defined"));
			return false;
		}
		cbNone();
	break;
	case NFC_P2P_PN532_CMD_RFREGULATIONTEST:
	break;
	case NFC_P2P_PN532_CMD_INJUMPFORDEP:
		if (cbInJumpFor == NULL) {
			//Serial.println(F("[nfc_p2p::validateReceivedInformation] callback not defined"));
			return false;
		}
		cbInJumpFor(buffer[2], buffer[3],&buffer[4], buffer[14], buffer[15], buffer[16], buffer[17], buffer[18], buffer[19], &buffer[19]);
	break;
	case NFC_P2P_PN532_CMD_INJUMPFORPSL:
	break;
	case NFC_P2P_PN532_CMD_INLISTPASSIVETARGET:
	break;
	case NFC_P2P_PN532_CMD_INATR:
	break;
	case NFC_P2P_PN532_CMD_INPSL:
	break;
	case NFC_P2P_PN532_CMD_INDATAEXCHANGE:
		if (cbReceive == NULL) {
			//Serial.println(F("[nfc_p2p::validateReceivedInformation] callback not defined"));
			return false;
		}
		cbReceive(buffer[2],len-3, &buffer[3]);
	break;
	case NFC_P2P_PN532_CMD_INCOMMUNICATETHRU:
	break;
	case NFC_P2P_PN532_CMD_INDESELECT:
	
	break;
	case NFC_P2P_PN532_CMD_INRELEASE:
	if (cbByte == NULL) {
			//Serial.println(F("[nfc_p2p::validateReceivedInformation] callback not defined"));
			return false;
		}
		cbByte(buffer[2]);
	break;
	case NFC_P2P_PN532_CMD_INSELECT:
	break;
	case NFC_P2P_PN532_CMD_INAUTOPOLL:
	break;
	case NFC_P2P_PN532_CMD_TGINITASTARGET:
		if (cbTgGetInit == NULL) {
			//Serial.println(F("[nfc_p2p::validateReceivedInformation] callback not defined"));
			return false;
		}
		cbTgGetInit(buffer[2], len-3, &buffer[3]);
	break;
	case NFC_P2P_PN532_CMD_TGSETGENERALBYTES:
	break;
	case NFC_P2P_PN532_CMD_TGGETDATA:
		if (cbReceive == NULL) {
			//Serial.println(F("[nfc_p2p::validateReceivedInformation] callback not defined"));
			return false;
		}
		cbReceive(buffer[2],len-3,&buffer[3]);
		
	break;
	case NFC_P2P_PN532_CMD_TGSETDATA:
		if (cbByte == NULL) {
			//Serial.println(F("[nfc_p2p::validateReceivedInformation] callback not defined"));
			return false;
		}
		cbByte(buffer[2]);
	break;
	case NFC_P2P_PN532_CMD_TGSETMETADATA:
	break;
	case NFC_P2P_PN532_CMD_TGGETINITIATORCOMMAND:
	break;
	case NFC_P2P_PN532_CMD_TGRESPONSETOINITIATOR:
	break;
	case NFC_P2P_PN532_CMD_TGGETTARGETSTATUS:
	break;
	default:
		// error. unexpected command
		//Serial.println(F("[nfc_p2p::validateReceivedInformation] error: command not implemented"));
		return false;
	}

	return true;
}

boolean nfc_p2p::executeCommand(uint8_t cmd) {
	if (busy)
		return false;

	lastCommand = cmd;
	spiBegin();
	spiDataWriting(lenTransmitBuffer, transmitBuffer);
	// wait for ack
	uint8_t frameType = spiDataReading(NFC_P2P_DEFAULT_TIMEOUT);
	spiEnd();
	resetAllCallbacks();
	memset(transmitBuffer, 0, sizeof(transmitBuffer));
	if (frameType==NFC_P2P_PN532_FRAME_ACK) {
		busy = true;
		//Serial.println("[nfc_p2p::executeCommand] busy->true");
		return true;
	} else
		return false;
}

uint8_t nfc_p2p::update() {
	if (hasData()) {
		//
		//Serial.println(F("[nfc_p2p::update] has data"));
		switch (spiDataReading(NFC_P2P_DEFAULT_TIMEOUT)) {
		case NFC_P2P_PN532_FRAME_NORMAL:
		case NFC_P2P_PN532_FRAME_EXTENDED:
			// normal and extended information frames are already handled by spiDataReading
			return NFC_P2P_UPDATE_OK;
		case NFC_P2P_PN532_FRAME_NACK:
		case NFC_P2P_PN532_FRAME_ACK:
			// shouldn't happen; NACK and ACK are received immediately after executing a command
			return NFC_P2P_UPDATE_UNEXPECTED;
			break;
		case NFC_P2P_PN532_FRAME_INVALID:
			return NFC_P2P_UPDATE_INVALIDFRAME;
		default:
			// shouldn't happen
			return NFC_P2P_UPDATE_INTERNALERROR;
		}
	}	
}


void nfc_p2p::spiDataWriting(uint16_t len, uint8_t *dataOut) {
	uint16_t i;
	spiBegin();
	spiWriteByte(NFC_P2P_SPI_DATAWRITING);
	for (i = 0; i < len; i++) {
		// delay?
		spiWriteByte(dataOut[i]);
	}
	spiEnd();
}

uint8_t nfc_p2p::calculateChecksum(uint8_t tfi, uint16_t len, uint8_t* data) {
	uint16_t i;
	uint8_t dcs=tfi;
	for (i = 0; i < len; i++)
		dcs += data[i];
	return ~dcs + 1;
}

void nfc_p2p::assembleNormalInformationFrame(uint8_t len, uint8_t* dataOut) {
	// writes normal information frame to the transmit buffer
	uint8_t i=0, j;
	transmitBuffer[i] = NFC_P2P_PN532_PREAMBLE; i++;
	transmitBuffer[i] = NFC_P2P_PN532_STARTCODE1; i++;
	transmitBuffer[i] = NFC_P2P_PN532_STARTCODE2; i++;
	transmitBuffer[i] = len+1; i++;
	transmitBuffer[i] = ~(len+1)+1; i++;
	transmitBuffer[i] = NFC_P2P_PN532_HOSTTOPN532; i++;
	for (j = 0; j < len; j++) {
		transmitBuffer[i] = dataOut[j]; i++;
	}
	transmitBuffer[i] = calculateChecksum(NFC_P2P_PN532_HOSTTOPN532, len, dataOut); i++;
	transmitBuffer[i] = NFC_P2P_PN532_POSTAMBLE; i++;
	lenTransmitBuffer = i;
}

void nfc_p2p::assembleExtendedInformationFrame(uint16_t len, uint8_t* dataOut) {
	// writes extended information frame to the transmit buffer
	uint16_t i = 0, j;
	transmitBuffer[i] = NFC_P2P_PN532_PREAMBLE; i++;
	transmitBuffer[i] = NFC_P2P_PN532_STARTCODE1; i++;
	transmitBuffer[i] = NFC_P2P_PN532_STARTCODE2; i++;
	transmitBuffer[i] = len + 1; i++;
	transmitBuffer[i] = ~(len + 1) + 1; i++;
	transmitBuffer[i] = NFC_P2P_PN532_HOSTTOPN532; i++;
	for (j = 0; j < len; j++) {
		transmitBuffer[i] = dataOut[j]; i++;
	}
	transmitBuffer[i] = calculateChecksum(NFC_P2P_PN532_HOSTTOPN532, len, dataOut); i++;
	transmitBuffer[i] = NFC_P2P_PN532_POSTAMBLE; i++;
	lenTransmitBuffer = i;
}

void nfc_p2p::assembleInformationFrame(uint16_t len, uint8_t* dataOut) {
	if (len < 255)
		assembleNormalInformationFrame(len, dataOut);
	else
		assembleExtendedInformationFrame(len, dataOut);
}

void nfc_p2p::resetAllCallbacks() {
	cbGetFirmwareVersion = NULL;
	cbNone = NULL;
	cbByte = NULL;
	cbInJumpFor = NULL;
	cbInListPassiveTarget = NULL;
	cbInAtr = NULL;
	cbReceive = NULL;
	cbInAutoPoll = NULL;
	cbTgGetInit = NULL;
	cbTgGetTargetStatus = NULL;
	cbBytes = NULL;
	cbReadGpio = NULL;
	ehByte = NULL;
}

boolean nfc_p2p::GetFirmwareVersion(t_cbGetFirmwareVersion cb) {
	boolean result;
	uint8_t cmd = NFC_P2P_PN532_CMD_GETFIRMWAREVERSION;
	assembleNormalInformationFrame(1, &cmd);
	result = executeCommand(NFC_P2P_PN532_CMD_GETFIRMWAREVERSION);
	cbGetFirmwareVersion = cb;
	return result;
}

boolean nfc_p2p::SAMConfiguration(uint8_t mode, uint8_t timeout, uint8_t irq, t_cbNone cb){
	boolean result;
	uint8_t i=0;
	uint8_t cmd[4];
	cmd[0] = NFC_P2P_PN532_CMD_SAMCONFIGURATION;
	cmd[1] = mode;
	cmd[2] = timeout;
	cmd[3] = irq;
	assembleNormalInformationFrame(4, cmd);
	result = executeCommand(NFC_P2P_PN532_CMD_SAMCONFIGURATION);
	cbNone = cb;
	return result;
}

boolean nfc_p2p::InJumpForDEP(boolean activeMode, uint8_t baudRate,uint8_t next, uint8_t lenPassiveInitiatorData, uint8_t* passiveInitiatorData, uint8_t lenNfcId3i, uint8_t* nfcId3i, uint8_t lenGeneralBytes, uint8_t* generalBytes, t_cbInJumpFor cb) {
	uint8_t cmd[4 + lenPassiveInitiatorData + lenNfcId3i + lenGeneralBytes];
	uint8_t i = 0,temp;
	boolean result;
	cmd[i++] = NFC_P2P_PN532_CMD_INJUMPFORDEP;
	cmd[i++] = activeMode;
	cmd[i++] = baudRate;
	cmd[i++] = next;
	uint8_t j = 0;
	temp = i;
	for (i; i < lenPassiveInitiatorData+temp; i++)
	{
		cmd[i] = passiveInitiatorData[j];
		j++;
	}
	j = 0;
	temp = i;
	for (i; i < lenNfcId3i + temp; i++)
	{
		cmd[i] = nfcId3i[j];
		j++;
	}
	j = 0;
	temp = i;
	for (i; i < lenGeneralBytes + temp; i++)
	{
		cmd[i] = generalBytes[j];
		j++;
	}
	//for (i = 0; i < sizeof(cmd) / sizeof(cmd[0]); i++) { Serial.println(cmd[i],HEX);}
	assembleNormalInformationFrame(sizeof(cmd) / sizeof(cmd[0]), cmd);
	result = executeCommand(NFC_P2P_PN532_CMD_INJUMPFORDEP);
	cbInJumpFor = cb;
	return result;
}

boolean nfc_p2p::TgInitAsTarget(uint8_t mode, uint8_t mifareParams[6], uint8_t felicaParams[18], uint8_t nfcId3t[10], uint8_t lenGeneralBytes, uint8_t* generalBytes, uint8_t lenHistoricalBytes, uint8_t* historicalBytes, t_cbTgGetInit cb) {
	uint8_t lenGeneralBytesTemp;
	uint8_t	lenHistoricalBytesTemp;

	if (lenGeneralBytes == 0)
	{
		lenGeneralBytesTemp = 1;
	}
	else {
		lenGeneralBytesTemp = lenGeneralBytes+1;
	}

	if (lenHistoricalBytes == 0)
	{
		lenHistoricalBytesTemp = 1;
	}
	else {
		lenHistoricalBytesTemp = lenHistoricalBytes+1;
	}
	
	uint8_t cmd[2 + 6 + 18 + 10 + lenGeneralBytesTemp + lenHistoricalBytesTemp];
	uint8_t i = 0, temp;
	boolean result;
	cmd[i++] = NFC_P2P_PN532_CMD_TGINITASTARGET;
	cmd[i++] = mode;
	uint8_t j = 0;
	temp = i;
	for (i; i < 6 + temp; i++)
	{
		cmd[i] = mifareParams[j];
		j++;
	}
	 j = 0;
	 temp = i;
	for (i; i < 18 + temp; i++)
	{
		cmd[i] = felicaParams[j];
		j++;
	}
	j = 0;
	temp = i;
	for (i; i < 10 + temp; i++)
	{
		cmd[i] = nfcId3t[j];
		j++;
	}
	j = 0;
	temp = i;
	if (lenGeneralBytes == 0) {
		cmd[temp] = 0;
		i++;
		temp = i;
	}
	else {
		cmd[temp] = lenGeneralBytes;
		i++;
		temp = i;
		
		for (i; i < lenGeneralBytes + temp; i++)
		{
			cmd[i] = generalBytes[j];
			j++;
		}
		temp = i;
	}
	//Serial.println(temp);
	j = 0;
	if (lenHistoricalBytes == 0) {
		cmd[temp] = 0;
	}
	else {
		cmd[temp] = lenHistoricalBytes;
		temp++;
		i++;
		for (i; i < lenHistoricalBytes + temp; i++)
		{
			cmd[i] = historicalBytes[j];
			j++;
		}
	}
	//for (i = 0; i < sizeof(cmd) / sizeof(cmd[0]); i++) {
		//Serial.println(cmd[i], HEX);
	//}
	assembleNormalInformationFrame(sizeof(cmd) / sizeof(cmd[0]), cmd);
	result = executeCommand(NFC_P2P_PN532_CMD_TGINITASTARGET);
	cbTgGetInit = cb;
	return result;

	
}

boolean nfc_p2p::InDataExchange(uint8_t relevantTarget, uint16_t lenDataOut, uint8_t* dataOut, t_cbReceive cb) {
	uint8_t cmd[2 + lenDataOut];
	uint8_t i = 0, temp;
	boolean result;
	cmd[i++] = NFC_P2P_PN532_CMD_INDATAEXCHANGE;
	cmd[i++] = relevantTarget;

	uint8_t j = 0;
	temp = i;
	for (i; i < lenDataOut + temp; i++)
	{
		cmd[i] = dataOut[j];
		j++;
	}
	assembleNormalInformationFrame(sizeof(cmd) / sizeof(cmd[0]), cmd);
	result = executeCommand(NFC_P2P_PN532_CMD_INDATAEXCHANGE);
	cbReceive = cb;
	//for (i = 0; i < sizeof(cmd) / sizeof(cmd[0]); i++) { Serial.println(cmd[i], HEX);}
	return result;
}

boolean nfc_p2p::TgGetData(t_cbReceive cb) {
	boolean result;
	uint8_t cmd[1]; 
	cmd[0]= NFC_P2P_PN532_CMD_TGGETDATA;
	assembleNormalInformationFrame(sizeof(cmd) / sizeof(cmd[0]), cmd);
	result = executeCommand(NFC_P2P_PN532_CMD_TGGETDATA);
	cbReceive = cb;
	return result;
}

boolean nfc_p2p::TgSetData(uint16_t lenDataOut, uint8_t* dataOut, t_cbByte cb) {
	boolean result;
	uint8_t cmd[1 + lenDataOut];
	uint8_t i = 0, temp;
	cmd[i++] = NFC_P2P_PN532_CMD_TGSETDATA;
	uint8_t j = 0;
	temp = i;
	for (i; i < lenDataOut + temp; i++)
	{
		cmd[i] = dataOut[j];
		j++;
	}
	assembleNormalInformationFrame(sizeof(cmd) / sizeof(cmd[0]), cmd);
	result = executeCommand(NFC_P2P_PN532_CMD_TGSETDATA);
	cbByte = cb;
	return result;
}

boolean nfc_p2p::RFConfiguration(uint8_t configurationItem, uint8_t* configurationData, t_cbNone cb) {
	boolean result;
	uint8_t cmd[5];
	cmd[0] = NFC_P2P_PN532_CMD_RFCOMMUNICATION;
	cmd[1] = configurationItem;
	cmd[2] = configurationData[0];
	cmd[3] = configurationData[1];
	cmd[4] = configurationData[2];
	assembleNormalInformationFrame(sizeof(cmd) / sizeof(cmd[0]), cmd);

	result = executeCommand(NFC_P2P_PN532_CMD_RFCOMMUNICATION);

	cbNone = cb;
	return result;
}

boolean nfc_p2p::InRelease(uint8_t relevantTarget, t_cbByte cb){
	
	boolean result;
	uint8_t cmd[5];
	cmd[0] = NFC_P2P_PN532_CMD_INRELEASE;
	cmd[1] = relevantTarget;
	
	assembleNormalInformationFrame(sizeof(cmd) / sizeof(cmd[0]), cmd);

	result = executeCommand(NFC_P2P_PN532_CMD_INRELEASE);

	cbByte = cb;
	return result;
}

boolean nfc_p2p::abortPreviousCommand(void) // to abort the previous command, you can just send another command
{
	boolean result;
	busy = false;
	result = true;
	return result;
}
