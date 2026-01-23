cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dccex-throttle)

target_sources(
	app PRIVATE
	src/banner.c
	src/buttons.c
	src/dcc.c
	src/leds.c
	src/poti.c
	src/queue.c
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
