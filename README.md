# extra-sensory
basic sensor project for esp8266 and freertos

To build, follow the instructions to get
[esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos).

When you can build the examples in that project, you can build this project by
cloning the repository, adding an environment variable:

    export RTOS_DIR=<path-to-rtos>

and then you can use make from the root directory of this repo according to the
esp-open-rtos instructions. For instance, to make this code and flash it to an
ESP (after checking that the ESPPORT is correct), you could use:

    make flash -j4 ESPPORT=/dev/ttyUSB0

## Notes:

Much of this code is just copied verbatim from examples in esp-open-rtos and
rearranged for modularity.
