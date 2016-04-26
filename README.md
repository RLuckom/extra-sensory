# extra-sensory
basic sensor project for esp8266 and freertos

To build, follow the instructions to get
[esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos).

When you can build the examples in that project, you can build this project by
cloning the repository, adding an environment variable:

    export RTOS_DIR=<path-to-rtos>

and then you can use make from the root directory of this repo:

    make flash -j4 ESPPORT=/dev/ttyUSB0
