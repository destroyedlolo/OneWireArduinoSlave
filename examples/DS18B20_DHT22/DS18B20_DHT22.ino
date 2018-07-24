/* Emulates a DS18B20 using a DHT22
 * 
 * Provided by Destroyedlolo. (http://destroyedlolo.info)
 *
 * 23/03/2018 - First version
 */

#include <Arduino.h>
#include <LowLevel.h>
#include <OneWireSlave.h>

Pin oneWireData(2);	// Where the 1-wire bus is connected to

const byte owROM[7] = { 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 };

enum OW_Cmd {
	START_CONVERSION = 0x44,
	WRITE_SCRATCHPAD = 0x4e,
	READ_POWERSUPPLY = 0xb4,
	READ_SCRATCHPAD = 0xBE
};

volatile byte scratchpad[9];

	/*
	 * DHT22 related
	 */
#include <SimpleDHT.h>	// https://github.com/winlinvip/SimpleDHT

SimpleDHT22 DHT;
#define pinDHT 3

	/*
	 * Let's go
	 */
#define DEBUG	// Comment out to be silence

enum DeviceState {
	WAIT4RESET,
	WAIT4COMMAND,
	CONVERTING,
	TEMPERATUREREADY
};

volatile DeviceState state = DeviceState::WAIT4RESET;
volatile unsigned long conversionStartTime = 0;

void owRcv( OneWireSlave::ReceiveEvent evt, byte cmd ){
	switch( evt ){
	case OneWireSlave::RE_Reset:
		state = DeviceState::WAIT4COMMAND;
#ifdef DEBUG
		Serial.println(F("Reset"));
#endif
		break;
	case OneWireSlave::RE_Error:
		state = DeviceState::WAIT4RESET;
#ifdef DEBUG
		Serial.println(F("Error"));
#endif
		break;
	case OneWireSlave::RE_Byte:
		if( state == DeviceState::WAIT4COMMAND ) switch( cmd ){
		case OW_Cmd::START_CONVERSION:
				// Do a new request only if enough time passed
			if( !conversionStartTime || millis() < conversionStartTime || millis() > conversionStartTime + 2000 ){
				state = DeviceState::CONVERTING;
				OWSlave.beginWriteBit(0, true); // send zeros as long as the conversion is not finished
#ifdef DEBUG
				Serial.println(F("Starting conversion"));
#endif
			}
#ifdef DEBUG
			else
				Serial.println(F("Ignored StartConv"));
#endif
			break;
		case OW_Cmd::READ_SCRATCHPAD:
#ifdef DEBUG
				Serial.println(F("Reading scratchpad"));
#endif
			state = DeviceState::WAIT4RESET;
			OWSlave.beginWrite((const byte*)scratchpad, 9, 0);
			break;
		case OW_Cmd::WRITE_SCRATCHPAD:
		case OW_Cmd::READ_POWERSUPPLY:
			state = DeviceState::WAIT4RESET;	// Ignore parameters
			break;
		default:
			state = DeviceState::WAIT4RESET;
#ifdef DEBUG
			Serial.print(F("Unknown command :"));
			Serial.println( cmd, HEX );
#endif
		}
	}
}

void setTemperature( float temp ){	// Write given temperature to the scratchpad
	int16_t raw = (int16_t)(temp * 16.0f + 0.5);
		// We don't care about race condition as well as only one command
		// can be processed at a time otherwise we are failing in error/collision
		// condition.
	scratchpad[0] = (byte)raw;
	scratchpad[1] = (byte)(raw >> 8);
	scratchpad[8] = OWSlave.crc8((const byte*)scratchpad, 8);
}

void setup(){
	Serial.begin(115200);	// debugging

	for(int i = 2; i<8; i++)
		scratchpad[i] = 0;
	setTemperature( 85 );

	OWSlave.setReceiveCallback(&owRcv);
	OWSlave.begin(owROM, oneWireData.getPinNumber());
}


void loop(){
	delay(10);

	if(state == DeviceState::CONVERTING){	// start conversion
		float temperature = 0;
		float humidite = 0;
		int err;

#ifdef DEBUG
		Serial.print(F("Converting ... "));
#endif

		conversionStartTime = millis();
		if((err = DHT.read2(pinDHT, &temperature, &humidite, NULL)) != SimpleDHTErrSuccess){
#ifdef DEBUG
			Serial.print(F("Error while reading,\nerr="));
			Serial.print(err);
			switch(err){
			case SimpleDHTErrStartLow :
				Serial.println(F(" - Error to wait for start low signal"));
				break;
			case SimpleDHTErrStartHigh :
				Serial.println(F(" - Error to wait for start high signal"));
				break;
			case SimpleDHTErrDataLow :
				Serial.println(F(" - Error to wait for data start low signal"));
				break;
			case SimpleDHTErrDataRead :
				Serial.println(F(" - Error to wait for data read signal"));
				break;
			case SimpleDHTErrDataEOF :
				Serial.println(F(" - Error to wait for data EOF signal"));
				break;
			case SimpleDHTErrDataChecksum :
				Serial.println(F(" - Error to validate the checksum"));
				break;
			case SimpleDHTErrZeroSamples :
				Serial.println(F(" - Error when temperature and humidity are zero, it shouldn't happen"));
				break;
			default :
				Serial.println(F(" - who know ?"));
				break;
			};
#endif
			temperature = 85;
		} else {
#ifdef DEBUG
			Serial.println(temperature);
#endif
		}
		setTemperature( temperature );
		state = DeviceState::TEMPERATUREREADY;
		OWSlave.beginWriteBit(1, true);
	}
}
