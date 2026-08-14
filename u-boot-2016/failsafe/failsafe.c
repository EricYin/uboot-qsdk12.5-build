// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 chenxin527. All Rights Reserved.
 *
 * This file is part of the project uboot-qsdk12.5-build
 *
 * Failsafe main entry point
 */

#include <common.h>
#include <command.h>
#include <console.h>
#include <malloc.h>
#include <version.h>
#include <linux/sizes.h>
#include <net/tcp.h>
#include <net/httpd.h>
#include <failsafe/failsafe.h>
#include <ipq_api.h>
#ifdef CONFIG_DHCPD
#include <net/dhcpd.h>
#endif /* CONFIG_DHCPD */
#ifdef CONFIG_TELNETD
#include <net/telnetd.h>
#endif /* CONFIG_TELNETD */
#ifdef CONFIG_NET_ABORT
#include <net_abort.h>
#endif /* CONFIG_NET_ABORT */

#include "fs.h"
#include "modules/modules.h"

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_HTTPD_DEBUG
bool httpd_debug_on;
#endif /* CONFIG_HTTPD_DEBUG */

bool httpd_running = false;
bool tcp_done = false;

extern bool auto_action_pending;
extern const void *upload_data;
extern upgrade_type_t upgrade_type;

typedef void (*register_uri_handlers_func_t)(struct httpd_instance *inst);

register_uri_handlers_func_t register_uri_handlers_funcs[] = {
	index_register_uri_handlers,
	html_register_uri_handlers,
	backup_register_uri_handlers,
	env_register_uri_handlers,
#ifdef CONFIG_FAILSAFE_MAC_MANAGEMENT
	mac_register_uri_handlers,
#endif /* CONFIG_FAILSAFE_MAC_MANAGEMENT */
	mibib_register_uri_handlers,
	misc_register_uri_handlers,
	network_register_uri_handlers,
	sysinfo_register_uri_handlers,
	syslog_register_uri_handlers,
	upgrade_register_uri_handlers,
	webterm_register_uri_handlers,
};

bool httpd_is_running(void)
{
	return httpd_running;
}

void print_eth_init_halt_skip_hint(int init)
{
	printf("httpd is running, skipping eth_%s(). "
		"If this is not the case, run `httpd stop` first\n",
		init ? "init" : "halt");
}

static void print_greeting_message(void)
{
	u32 ip = ntohl(net_ip.s_addr);

	printf("\nWeb failsafe UI started\n");
	printf("URL: http://%u.%u.%u.%u/\n",
	       (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
	       (ip >> 8) & 0xFF, ip & 0xFF);
	printf("\nPress Ctrl+C to exit\n\n");
}

static int failsafe_loop(void)
{
	int ret;

	net_init();
	if (eth_is_on_demand_init()) {
		eth_halt();
		eth_set_current();
		ret = eth_init();
		while (ret < 0) {
			ulong ts = get_timer(0);
			do {
				if (ctrlc()) {
					eth_halt();
					return ret;
				}
				udelay(10000);
			} while (get_timer(ts) < 1000);
			ret = eth_init();
		}
		if (ret < 0) {
			eth_halt();
			return ret;
		}
	} else {
		eth_init_state_only();
	}

#ifdef CONFIG_NET_ABORT
	if (net_abort_detected()) {
		/* 多发送几次回复消息，确保客户端能收到 */
		for (int i = 0; i < 5; i++) {
			net_abort_reply();
			mdelay(200);
		}
		net_abort_state_reset();
	}
#endif

	httpd_running = true;
	tcp_done = false;
	auto_action_pending = false;

	tcp_reset_all_conn();

	tcp_start();

#ifdef CONFIG_DHCPD
	dhcpd_start();
#endif
#ifdef CONFIG_TELNETD
	if (get_enable_state("telnet_enable", true))
		telnetd_start(telnetd_get_port_from_env());
#endif

	while (!ctrlc() && !tcp_done && !auto_action_pending) {
		eth_rx();
		if (tcp_periodic_check())
			tcp_done = true;
	}

#ifdef CONFIG_DHCPD
	if (dhcpd_is_running())
		dhcpd_stop();
#endif
#ifdef CONFIG_TELNETD
	if (telnetd_is_running())
		telnetd_stop();
#endif

	httpd_running = false;
	tcp_reset_all_conn();
	eth_halt();

	return 0;
}

int start_web_failsafe(void)
{
	struct httpd_instance *inst;
	int ret;

	inst = httpd_find_instance(80);
	if (inst)
		httpd_free_instance(inst);

	inst = httpd_create_instance(80);
	if (!inst) {
		printf("Error: failed to create HTTP instance on port 80\n");
		return -1;
	}

	print_greeting_message();
	handle_start_led_state();

	for (int i = 0; i < ARRAY_SIZE(register_uri_handlers_funcs); i++)
		(register_uri_handlers_funcs[i])(inst);

	ret = failsafe_loop();

	inst = httpd_find_instance(80);
	if (inst)
		httpd_free_instance(inst);

	return ret ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

static int do_httpd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "stop")) {
			httpd_running = false;
			puts("httpd stopped.\n");
			return CMD_RET_SUCCESS;
		} else if (strcmp(argv[1], "start")) {
			printf("Error: unknown argument: %s\n", argv[1]);
			return CMD_RET_USAGE;
		}
	}

#ifdef CONFIG_HTTPD_DEBUG
	httpd_debug_on = get_enable_state("httpd_debug", true);
#endif

#ifdef CONFIG_NET_FORCE_IPADDR
	net_ip = string_to_ip(__stringify(CONFIG_IPADDR));
	net_netmask = string_to_ip(__stringify(CONFIG_NETMASK));
#endif

	ret = start_web_failsafe();

	if (auto_action_pending) {
		handle_success_led_state();
		mdelay(1000);
		if (upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_INITRAMFS)
			boot_from_mem((ulong)upload_data);
		else
			do_reset(NULL, 0, 0, NULL);
	} else {
		handle_fail_led_state();
	}

	return ret;
}

U_BOOT_CMD(httpd, 2, 0, do_httpd,
	"Failsafe HTTP Server\n"
	"httpd [start]     - start http server\n"
	"httpd stop        - stop http server (only set httpd_running flag to false)\n",
	""
);
