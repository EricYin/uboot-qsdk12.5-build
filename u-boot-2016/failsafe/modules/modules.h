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

#ifndef _FAILSAFE_MODULES_H_
#define _FAILSAFE_MODULES_H_

// =============================================================================
// 闪存备份
// =============================================================================
void backup_handler(enum httpd_uri_handler_status status,
        struct httpd_request *request,
        struct httpd_response *response);

// =============================================================================
// 环境变量管理
// =============================================================================
void env_list_handler(enum httpd_uri_handler_status status,
	    struct httpd_request *request,
	    struct httpd_response *response);
void env_set_handler(enum httpd_uri_handler_status status,
	    struct httpd_request *request,
	    struct httpd_response *response);
void env_unset_handler(enum httpd_uri_handler_status status,
	    struct httpd_request *request,
	    struct httpd_response *response);
void env_reset_all_handler(enum httpd_uri_handler_status status,
	    struct httpd_request *request,
	    struct httpd_response *response);
void env_reset_single_handler(enum httpd_uri_handler_status status,
	    struct httpd_request *request,
	    struct httpd_response *response);
void env_restore_handler(enum httpd_uri_handler_status status,
	    struct httpd_request *request,
	    struct httpd_response *response);

// =============================================================================
// MAC 管理
// =============================================================================
void mac_info_handler(enum httpd_uri_handler_status status,
	    struct httpd_request *request,
	    struct httpd_response *response);
void mac_set_handler(enum httpd_uri_handler_status status,
	    struct httpd_request *request,
	    struct httpd_response *response);

// =============================================================================
// MIBIB 重载
// =============================================================================
void mibib_reload_handler(enum httpd_uri_handler_status status,
        struct httpd_request *request,
        struct httpd_response *response);

// =============================================================================
// 网络参数设置
// =============================================================================
void network_info_handler(enum httpd_uri_handler_status status,
	    struct httpd_request *request,
	    struct httpd_response *response);
void network_set_handler(enum httpd_uri_handler_status status,
	    struct httpd_request *request,
	    struct httpd_response *response);
void network_reset_handler(enum httpd_uri_handler_status status,
	    struct httpd_request *request,
	    struct httpd_response *response);

// =============================================================================
// 系统信息
// =============================================================================
void sysinfo_handler(enum httpd_uri_handler_status status,
        struct httpd_request *request,
        struct httpd_response *response);

// =============================================================================
// 系统日志
// =============================================================================
void syslog_poll_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response);

// =============================================================================
// 网页终端
// =============================================================================
void webterm_exec_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response);
void webterm_upload_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response);
void webterm_cmdlist_handler(enum httpd_uri_handler_status status,
	struct httpd_request *request,
	struct httpd_response *response);

#endif /* _FAILSAFE_MODULES_H_ */
