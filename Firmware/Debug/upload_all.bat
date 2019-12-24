@set MK=m168
@set PRG=avr109
@set COM=COM41
@set FLASHFILE=KM_3200_v220.eep
@set AVRDUDEPATH=C:\WinAVR-20100110\bin\
 
REM FLASH
%AVRDUDEPATH%avrdude -p %MK% -c %PRG% -b 19200 -P %COM% -e -U eeprom:w:%FLASHFILE%:a

@set MK=m168
@set PRG=avr109
@set COM=COM41
@set FLASHFILE=KM_3200_v220.hex
@set AVRDUDEPATH=C:\WinAVR-20100110\bin\
 
REM FLASH
%AVRDUDEPATH%avrdude -p %MK% -c %PRG% -b 19200 -P %COM% -e -U flash:w:%FLASHFILE%:a

pause