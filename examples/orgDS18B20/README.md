This is the **DS18B20** fake emulator provided with the original version of OneWireArduinoSlave.

To make it working, connect :

- a GND pin of your Uno to your 1-wire network ground.
- the pin "2" of your Unto to the data line of your 1-wire network. You may use "3" as well but need to change the code.

You will see 28.000000000002 probe that always return 42 as temperature :

    $ cat 28.000000000002/temperature
    42
Notez-bien : conversion timing is emulated as well.
