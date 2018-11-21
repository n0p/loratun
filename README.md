# modemtun
generic TUN-modem interface implementation

This code creates a tun devices, processes incoming packets through a static header compression engine (SHCH) and forwards them to a modem interface.

The modem "modemfoo" is an empty modem, for use as an example implementation. The functions to be implemented on a modem are specified on /lib/modem.h

The modem "lorawan" interfaces with a "I-CUBE-LRWAN" modem firmware, as provided by ST micro:

https://www.st.com/en/embedded-software/i-cube-lrwan.html

## License
The /schc/ library is under a 3-clause BSD license, for more information contact jesus.sanchez4 at um.es, 

The rest of the code is under the same license when compiled together OR under GPLv3 when not compiled with the code on /schc/

## Building
Easy peasy...

```
make
```

This will generate ```footun``` and ```loratun``` binaries.

To rebuild, ```make clean && make```.



## Testing

To test the modemfoo dummy echo modem:


```
make test_footun
```

To test the loratun LoRaWAN modem:

```
make test_loratun
```
