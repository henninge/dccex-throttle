cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dccex-throttle)

target_sources(
	app PRIVATE
	src/poti.c
	src/queue.c
	src/dcc.c
	src/buttons.c
	src/leds.c
	src/state.c
)

target_sources_ifdef(CONFIG_WIFI
	app PRIVATE
	src/wifi_connect.c
)

target_sources_ifdef(CONFIG_SHIELD_USB_ACM_SERIAL
	app PRIVATE
	src/console.c
)
