// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 chenxin527. All Rights Reserved.
 *
 * This file is part of the project uboot-qsdk12.5-build
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <net/httpd.h>
#include <ipq_api.h>
#include <mmc.h>
#include <sdhci.h>
#include <part.h>
#include <u-boot/md5.h>
#include <failsafe/fw_dec.h>
#include <capture.h>

#include "modules.h"

#ifndef CONFIG_SDHCI_SUPPORT
extern qca_mmc mmc_host;
#else
extern struct sdhci_host mmc_host;
#endif

#define WEBTERM_MAX_CMD_SIZE			256
#define WEBTERM_RECORD_OUT_SIZE			16666
#define WEBTERM_UPLOAD_FILE_INFO_SIZE	999

/* Implemented in u-boot-2016/common/cli_hush.c */
extern bool is_last_command_repeatable(void);
extern void webterm_init_repeat_flag(void);
extern void webterm_repeat_last_command(bool repeat);

struct webterm_exec_session {
	char resp_buf[2048];
	char *output_buf;
	size_t output_buf_size;
	struct httpd_form_value *raw_cmd;
	bool body_sent;
};

static bool webterm_exec_resp_custom;

static struct {
	char data[WEBTERM_MAX_CMD_SIZE + 1];
	bool repeatable;
} command = { .repeatable = false };

static void handle_response_message(struct httpd_response *response,
    int code, const char *data, int data_size, const char *content_type)
{
	response->status = HTTP_RESP_STD;
	response->data = data ? data : "";
	response->size = (data_size != -1) ? data_size : strlen(response->data);
	response->info.code = code;
	response->info.connection_close = 1;
	response->info.content_type = content_type ? content_type : "text/plain";
}

static void webterm_free_session_data(struct httpd_response *response)
{
	if (response->session_data) {
		free(response->session_data);
		response->session_data = NULL;
	}
}

static int webterm_run_command(void *cmd)
{
	return run_command((const char *)cmd, 0);
}

static void webterm_exec_handler(enum httpd_uri_handler_status status,
		struct httpd_request *request,
		struct httpd_response *response)
{
	struct httpd_form_value *raw_cmd;
	struct webterm_exec_session *sess;
	const char *echo_str = "";
	const char *do_repeat_str = "__DO_REPEAT__";
	const char *cancel_repeat_str = "__CANCEL_REPEAT__";
	char *output_buf;
	size_t output_buf_size = WEBTERM_RECORD_OUT_SIZE;
	size_t out_len = 0;
	int ret;

	if (status == HTTP_CB_NEW) {
		webterm_exec_resp_custom = false;
		response->session_data = NULL;

		if (!request || request->method != HTTP_POST) {
			handle_response_message(response, 405, "bad method", -1, NULL);
			return;
		}

		raw_cmd = httpd_request_find_value(request, "cmd");
		if (!raw_cmd || !raw_cmd->data || !raw_cmd->size) {
			handle_response_message(response, 400, "no cmd", -1, NULL);
			return;
		}

		sess = calloc(1, sizeof(*sess));
		if (!sess) {
			handle_response_message(response, 500, "no mem for session data", -1, NULL);
			return;
		}

		output_buf = malloc(output_buf_size);
		if (!output_buf) {
			free(sess);
			handle_response_message(response, 500, "no mem for output buffer", -1, NULL);
			return;
		}

		webterm_exec_resp_custom = true;
		sess->raw_cmd = raw_cmd;
		sess->output_buf = output_buf;
		sess->output_buf_size = output_buf_size;
		sess->body_sent = false;

		response->session_data = sess;

		response->status = HTTP_RESP_CUSTOM;

		response->info.http_1_0 = 1;
		response->info.content_length = -1;
		response->info.connection_close = 1;
		response->info.content_type = "text/plain";
		response->info.code = 200;

		response->size = http_make_response_header(&response->info,
							sess->resp_buf, sizeof(sess->resp_buf));
		response->data = sess->resp_buf;

		return;
	}

	if (status == HTTP_CB_RESPONDING && webterm_exec_resp_custom) {
		sess = response->session_data;
		raw_cmd = sess->raw_cmd;
		output_buf = sess->output_buf;

		if (sess->body_sent) {
			response->status = HTTP_RESP_NONE;
			return;
		}

		if (raw_cmd->size == strlen(do_repeat_str) &&
				!strcmp(raw_cmd->data, do_repeat_str)) {
			if (!command.repeatable)
				goto resp_empty;
			webterm_repeat_last_command(true);
			echo_str = "<REPEAT>";
		} else if (raw_cmd->size == strlen(cancel_repeat_str) &&
				!strcmp(raw_cmd->data, cancel_repeat_str)) {
			command.repeatable = false;
			webterm_repeat_last_command(false);
			goto resp_empty;
		} else {
			strlcpy(command.data, raw_cmd->data, sizeof(command.data));
			webterm_init_repeat_flag();
			webterm_repeat_last_command(false);
			echo_str = command.data;
		}

		printf("\nIPQ# %s\n", echo_str);

		ret = call_func_capture(webterm_run_command, command.data,
				output_buf, output_buf_size, &out_len);

		command.repeatable = (!ret && is_last_command_repeatable()) ? true : false;

		response->data = output_buf;
		response->size = out_len;
		sess->body_sent = true;

		return;

	resp_empty:
		response->data = "";
		response->size = strlen(response->data);
		sess->body_sent = true;
		return;
	}

	if (status == HTTP_CB_CLOSED) {
		if (webterm_exec_resp_custom) {
			sess = response->session_data;
			if (sess && sess->output_buf) {
				free(sess->output_buf);
				sess->output_buf = NULL;
			}
		}
		webterm_free_session_data(response);
		return;
	}
}

static int print_file_info(void *arg)
{
	struct httpd_form_value *file;
	unsigned char md5_sum[16];
	const char *separator;
	fw_type_t fw_type;

	file = arg;
	fw_type = check_fw_type((uintptr_t)file->data, file->size);
	md5((unsigned char *)file->data, file->size, md5_sum);

	separator = "\n=================================="
				"=========================================\n";

	puts(separator);
	printf(" [FILE] %s\n", file->filename);
	printf(" [TYPE] %s\n", fw_type_to_string(fw_type));
	printf(" [ADDR] 0x%lx\n", (ulong)file->data);

	printf(" [SIZE] 0x%lx (", (ulong)file->size);
	print_size(file->size, ")\n");

	puts(" [ MD5] ");
	for (int i = 0; i < 16; i++)
		printf("%02x", md5_sum[i] & 0xFF);
	puts(separator);

	return 0;
}

static void webterm_upload_handler(enum httpd_uri_handler_status status,
		struct httpd_request *request,
		struct httpd_response *response)
{
	struct httpd_form_value *file;
	char *buf;
	size_t len = 0;

	if (status == HTTP_CB_CLOSED) {
		webterm_free_session_data(response);
		return;
	}

	if (status != HTTP_CB_NEW)
		return;

	response->session_data = NULL;

	file = httpd_request_find_value(request, "file");
	if (!file || !file->data) {
		handle_response_message(response, 400, "no file", -1, NULL);
		return;
	}

	set_file_info_env((ulong)file->data, (ulong)file->size);

	buf = malloc(WEBTERM_UPLOAD_FILE_INFO_SIZE);
	if (buf) {
		call_func_capture(print_file_info, file,
			buf, WEBTERM_UPLOAD_FILE_INFO_SIZE, &len);
		handle_response_message(response, 200, buf, len, NULL);
		response->session_data = buf;
	} else {
		handle_response_message(response, 200,
			"\n============================\n"
			" File uploaded successfully"
			"\n============================\n", -1, NULL);
	}
}

static void webterm_cmdlist_handler(enum httpd_uri_handler_status status,
		struct httpd_request *request,
		struct httpd_response *response)
{
	cmd_tbl_t *cmd_start = ll_entry_start(cmd_tbl_t, cmd);
	const int cmd_items = ll_entry_count(cmd_tbl_t, cmd);
	int len = 0, left = 6666;
	char *buf;
	char esc_cmd_usage[512];

	if (status == HTTP_CB_CLOSED) {
		webterm_free_session_data(response);
		return;
	}

	if (status != HTTP_CB_NEW)
		return;

	response->session_data = NULL;

	if (!request || request->method != HTTP_GET) {
		handle_response_message(response, 405, "bad method", -1, NULL);
		return;
	}

	buf = malloc(left);
	if (!buf) {
		handle_response_message(response, 500, "no mem", -1, NULL);
		return;
	}

	len += snprintf(buf + len, left - len, "{");
	len += snprintf(buf + len, left - len, "\"cmdlist\": [");

	for (int i = 0; i < cmd_items; i++) {
		json_escape(cmd_start[i].usage, esc_cmd_usage, sizeof(esc_cmd_usage));
		len += snprintf(buf + len, left - len,
			"%s{\"name\":\"%s\",\"usage\":\"%s\"}",
			i ? "," : "", cmd_start[i].name, esc_cmd_usage);
	}

	len += snprintf(buf + len, left - len, "]");
	len += snprintf(buf + len, left - len, "}");

	handle_response_message(response, 200, buf, -1, "application/json");
	response->session_data = buf;
}

void webterm_register_uri_handlers(struct httpd_instance *inst)
{
	httpd_register_uri_handler(inst, "/webterm/exec", &webterm_exec_handler, NULL);
	httpd_register_uri_handler(inst, "/webterm/upload", &webterm_upload_handler, NULL);
	httpd_register_uri_handler(inst, "/webterm/cmdlist", &webterm_cmdlist_handler, NULL);
}
