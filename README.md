# outside-temperature-sensor
Code to run on the nRF52832 from sparkfun  (https://www.sparkfun.com/products/13990)

This sensor measures the temperature and transmits that data in a bluetooth advertisement (beaconing)

The makefile supports the following commands:
 - make                    - Build the code
 - make flash_softdevice   - flash the softdevice onto the MCU
 - make sdk_config         - start external tool for editing sdk_config.h
 - make flash              - flashing binary

The following tools are required:
make
GNU embedded toolchain
nrfjprog

Improvements to be made:
- Move to cmake
- Make install script such that all tools are installed by default
  - make meta repo with this script and stuff
