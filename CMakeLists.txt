cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dccex-throttle)

target_sources(
	app PRIVATE
	src/poti.c
	src/buttons.c
	src/leds.c
)

target_sources_ifdef(CONFIG_SHIELD_USB_ACM_SERIAL
	app PRIVATE
	src/console.c
)
