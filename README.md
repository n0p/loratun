# modemtun
modem tunneled interface

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
