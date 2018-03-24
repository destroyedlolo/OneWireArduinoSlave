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

void owRcv( OneWireSlave::ReceiveEvent evt, byte cmd ){
	switch( evt ){
	case OneWireSlave::RE_Reset:
		state = DeviceState::WAIT4COMMAND;
		break;
	case OneWireSlave::RE_Error:
		state = DeviceState::WAIT4RESET;
		break;
	case OneWireSlave::RE_Byte:
		switch( cmd ){
		case OW_Cmd::START_CONVERSION:
			state = DeviceState::CONVERTING;
			OWSlave.beginWriteBit(0, true); // send zeros as long as the conversion is not finished
			// consequently, we don't have to take care of race condition as no other command can arrive
			break;
		case OW_Cmd::READ_SCRATCHPAD:
			state = DeviceState::WAIT4RESET;
			OWSlave.beginWrite((const byte*)scratchpad, 9, 0);
			break;
#ifdef DEBUG
		default:
			Serial.print(F("Unknown command :"));
			Serial.println( cmd, HEX );
#endif	
		}
	}
}

void setTemperature( float temp ){	// Write given temperature to the scratchpad
	int16_t raw = (int16_t)(temp * 16.0f + 0.5f);

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

		// No need to have a local copy of "state" as we are looking
		// for only one value
	if(state == DeviceState::CONVERTING){	// start conversion
		float temperature = 0;
		float humidite = 0;
		int err;

		if((err = DHT.read2(pinDHT, &temperature, &humidite, NULL)) != SimpleDHTErrSuccess){
#ifdef DEBUG
			Serial.print("\nError while reading, err=");
			Serial.println(err);
#endif
			temperature = 85;
		} else 
			setTemperature( temperature );
		state = DeviceState::TEMPERATUREREADY;
		OWSlave.beginWriteBit(1, true);
	}
}
