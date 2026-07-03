## Test Cmd

*Work with bk_idk cli interface, now not used*

### Alarm

**Get Current Time**

otAlarm get ms

otAlarm get us

**Stop Current Alarm**

otAlarm stop ms

otAlarm stop us

**Start Current Alarm**

otAlarm start ms *duration*

otAlarm start us *duration*

### Entropy

**Get Entropy Value**

otEntropy get

otEntropy get *length*

**Selt Test**

otEntropySelfTest

### Flash

**Flash Signle Cmd**

otFlash erase *swapIndex*

otFlash read *swapIndex* *offset* *size*

otFlash write *swapIndex* *offset* *sieze*

**Flash Self Test**

otFlashSelfTest

### Uart

**Uart Single Cmd**

otUart enable

otUart disable

otUart send

otUart flush


