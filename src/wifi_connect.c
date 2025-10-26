#include <zephyr/net/net_ip.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

#include <string.h>

#include "thread_prios.h"
#include "leds.h"


LOG_MODULE_REGISTER(WIFI);

static void wifi_config_init(struct wifi_connect_req_params *wifi_config);
static int wifi_connect_mgmt(struct net_if *wifi_if, struct wifi_connect_req_params *wifi_config);
static int wifi_wait_dhcp(struct net_if *wifi_if);
static int wifi_connect();


SYS_INIT(wifi_connect, APPLICATION, WIFI_CONNECT_PRIO);

static int wifi_connect() {
	static struct wifi_connect_req_params wifi_config;
	wifi_config_init(&wifi_config);

	struct net_if *wifi_if;
	wifi_if = net_if_get_wifi_sta();
	if (wifi_if == NULL) {
		LOG_ERR("Wifi interface not present");
		return -1;
	}

	LOG_INF("Connecting to SSID: %s", wifi_config.ssid);
	wifi_connect_mgmt(wifi_if, &wifi_config);
	wifi_wait_dhcp(wifi_if);

	LOG_INF("Wifi connected");
	leds_set_stage(STAGE_GREEN);
	return 0;
}

static void wifi_config_init(struct wifi_connect_req_params *wifi_config) {
	wifi_config->ssid = (const uint8_t *)CONFIG_WIFI_SSID;
	wifi_config->ssid_length = strlen(CONFIG_WIFI_SSID);
	wifi_config->psk = (const uint8_t *)CONFIG_WIFI_PASSWORD;
	wifi_config->psk_length = strlen(CONFIG_WIFI_PASSWORD);
	wifi_config->security = WIFI_SECURITY_TYPE_PSK;
	wifi_config->channel = WIFI_CHANNEL_ANY;
	wifi_config->band = WIFI_FREQ_BAND_2_4_GHZ;
	wifi_config->timeout = 120;
}

static int wifi_connect_mgmt(struct net_if *wifi_if, struct wifi_connect_req_params *wifi_config) {
	while (1) {
		int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, wifi_if, wifi_config,
				sizeof(struct wifi_connect_req_params));
		if (ret == 0) {
			break;
		}
		LOG_ERR("Unable to Connect to %s (%d). Retrying in 1s.", CONFIG_WIFI_SSID, ret);
		k_sleep(K_SECONDS(1));
	}
	return 0;
}

static int wifi_wait_dhcp(struct net_if *wifi_if) {
	struct in_addr *wifi_ip;

	while (1) {
		if (net_if_is_up(wifi_if)) {
			wifi_ip = net_if_ipv4_get_global_addr(wifi_if, NET_ADDR_ANY_STATE);
			if (wifi_ip != NULL) {
				LOG_DBG(
					"Wifi IP: %d.%d.%d.%d",
					wifi_ip->s4_addr[0], wifi_ip->s4_addr[1], wifi_ip->s4_addr[2], wifi_ip->s4_addr[3]
				);
				break;
			}
		} else {
			LOG_DBG("Wifi IF still down");
		}
		k_sleep(K_SECONDS(1));
	}
	return 0;
}

