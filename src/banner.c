#include <zephyr/kernel.h>

#include "thread_prios.h"

static int app_banner() {
    printk("DCC-EX Throttle (c) 2026 Henning Eggers\n");
    printk("built version %s\n", CONFIG_GIT_COMMIT);
    return 0;
}

SYS_INIT(app_banner, APPLICATION, APP_BANNER_PRIO);

