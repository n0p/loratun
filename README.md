# modemtun
modem tunneled interface

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
