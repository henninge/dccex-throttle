# DCC-EX Throttle

A throttle for [DCC-EX](https://dcc-ex.com/) implemented on [Zephyr](https://www.zephyrproject.org/) targeted for the [Raspberry Pi Pico 2 W](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html#pico2w-technical-specification).

## Background

A throttle is a device to control a model railroad locomotive's speed. In digital model railroading it can also control lights, sounds and other functions on the locomotive.

The DCC-EX command station is the device that drives that digital railroad. It includes
- An Arduino Mega board
- A motor schield that connects to the rails
- A WLAN board to connect to throttles for control
- The DCC-EX software installed from the website (in Arduino C++).

DCC-EX offers a text-based interface to receive commands from throttles and report status. It can be accessed via serial or WLAN (no bluetooth yet). There are apps like Engine Driver to use a smartphone for a throttle.

In my setup the command station opens it's own WLAN access point and I have an old Nexus phone with Engine Driver as a reference throttle. Engine Driver also allows logs it communication with the command station which is helpful for learning and debugging.

## Hardware setup

The hardware around the Raspberry Pico will consist of:
- A Potentiometer for speed control, attached to a lever.
- 3 illuminated arcade buttons for Forward, Backward and emergency stop
- a number of arcade buttons for controlling lights & sounds
- an RGB status led

In the first iteration the pico will simply run off an USB power bank.
