/* Emulates a DS18B20 using a SHT31
 * 
 * Provided by Destroyedlolo. (http://destroyedlolo.info)
 *
 * 09/09/2018 - First version
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
	 * SHT31 related
	 */

#include <Wire.h>
#include "Adafruit_SHT31.h"

Adafruit_SHT31 sht31;

	/*
	 * DS18B20 emulation
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

void setTemperature( float temp ){	// Write given temperature to the scratchpad
	int16_t raw = (int16_t)(temp * 16.0f);
		// We don't care about race condition as well as only one command
		// can be processed at a time otherwise we are failing in error/collision
		// condition.
	scratchpad[0] = (byte)raw;
	scratchpad[1] = (byte)(raw >> 8);
	scratchpad[4] = 0x7f;	// Ensure 12b responses
	scratchpad[8] = OWSlave.crc8((const byte*)scratchpad, 8);

#ifdef DEBUG
	Serial.print(F("SP : "));
	Serial.println( (float)(((scratchpad[1] << 8) + scratchpad[0] ) *0.0625) );
#endif
}

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
#ifdef DEBUG
			Serial.println(F("Ignoring WriteSP or ReadPS"));
#endif
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


	/*
	 * Let's go
	 */

void setup() {
	Serial.begin(9600);	// serial debugging

  	for(int i = 2; i<8; i++)	// Initial scratchpad value
		scratchpad[i] = 0;
	setTemperature( 85 );

	OWSlave.setReceiveCallback(&owRcv);
	OWSlave.begin(owROM, oneWireData.getPinNumber());

		/* Has to be set AFTER OWSlave init otherwise
		 * the arduino will hang
		 */
	sht31.begin(0x44);	// Start with the sht31

#ifdef DEBUG
	Serial.print(F("Ready, waiting for 1-wire command ... "));
#endif	
}


void loop() {
	delay(10);

	if(state == DeviceState::CONVERTING){	// start conversion
		float temperature = sht31.readTemperature();
//		float humidite = sht31.readHumidity();

#ifdef DEBUG
		Serial.print(F("Converting ... "));
#endif

		conversionStartTime = millis();
		if(isnan(temperature)){
			Serial.println(F("Failed to read temperature"));
			temperature = 85;
		}
#ifdef DEBUG
		else
			Serial.println(temperature);
#endif
		setTemperature( temperature );
		state = DeviceState::TEMPERATUREREADY;
		OWSlave.beginWriteBit(1, true);
	}
}
