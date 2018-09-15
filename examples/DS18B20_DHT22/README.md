This little code emulates a **DS18B20** with value read from a DHT22.

Connection :
------------

- **GND** pin a **GND** pin of your Uno to your 1-wire network ground as well as you DHT22 Gnd (pin **4**)
- the pin "**2**" of your Uno to the data line of your 1-wire network.
- **Vcc** pin of your Uno to DHT22 **Vcc** (pin **1**)
- pin "**3**" of your Uno to the data line of the DHT22 (pin **2**)

You will see 28.000000000002 probe :

    $ cat 28.000000000002/temperature
    26.5


** Notez-bien :** due to reliability issues related to the DHT22 itself as well as it's timing, this branch is stalling and not completely tested.

