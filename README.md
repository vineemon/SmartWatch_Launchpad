# SmartWatch_Launchpad (Voice Aware)

This project contains the source code for the firmware embedded on a TI CC2640R2F microcontroller. This project contains BLE logic as well as logic to interface with the flash memory on the Launchpad as well as an external DRV2605 Haptic Driver & Motor. 

simple_peripheral_cc2640r2lp_app/Application:

* Bluetooth Services (/services)
* Accelerometer Handling and Flash Memory Data Storage (accelerometer.c)
* Haptic Driver Calls (accelerometer.c)
* Bluetooth Stack Logic (simple_peripheral.c)
