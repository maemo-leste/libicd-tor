/*
 * This file is part of libicd-tor
 *
 * Copyright (C) 2021, Merlijn Wajer <merlijn@wizzup.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 3.0 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "libicd_network_tor.h"

/* XXX: Taken from ipv4 module */
gboolean string_equal(const char *a, const char *b)
{
	if (!a)
		return !b;

	if (b)
		return !strcmp(a, b);

	return FALSE;
}

tor_network_data *icd_tor_find_first_network_data(network_tor_private * private)
{
	GSList *l;

	for (l = private->network_data_list; l; l = l->next) {
		tor_network_data *found = (tor_network_data *) l->data;

		if (!found)
			TN_WARN("tor network data is NULL");
		else {
			return found;
		}
	}

	return NULL;
}

tor_network_data *icd_tor_find_network_data(const gchar * network_type,
					    guint network_attrs,
					    const gchar * network_id, network_tor_private * private)
{
	GSList *l;

	for (l = private->network_data_list; l; l = l->next) {
		tor_network_data *found = (tor_network_data *) l->data;

		if (!found)
			TN_WARN("tor network data is NULL");
		else {
			if (found->network_attrs == network_attrs &&
			    string_equal(found->network_type, network_type) &&
			    string_equal(found->network_id, network_id)) {
				return found;
			}
		}
	}

	return NULL;
}

/* pathname and arg are like in execv, returns pid, 0 is error */
pid_t spawn_as(const char *username, const char *pathname, char *args[])
{
	struct passwd *ent = getpwnam(username);
	if (ent == NULL) {
		TN_CRIT("spawn_as: getpwnam failed\n");
		return 0;
	}

	pid_t pid = fork();
	if (pid < 0) {
		TN_CRIT("spawn_as: fork() failed\n");
		return 0;
	} else if (pid == 0) {
		if (setgid(ent->pw_gid)) {
			TN_CRIT("setgid failed\n");
			exit(1);
		}
		if (setuid(ent->pw_uid)) {
			TN_CRIT("setuid failed\n");
			exit(1);
		}
		execv(pathname, args);

		TN_CRIT("execv failed\n");
		exit(1);
	} else {
		TN_DEBUG("spawn_as got pid: %d\n", pid);
		return pid;
	}

	return 0;		// Failure
}

void network_free_all(tor_network_data * network_data)
{
	network_tor_private *priv = network_data->private;
	if (priv->network_data_list) {
		priv->network_data_list = g_slist_remove(priv->network_data_list, network_data);
	}

	g_free(network_data->network_type);
	g_free(network_data->network_id);

	network_data->private = NULL;

	g_free(network_data);
}

void network_stop_all(tor_network_data * network_data)
{
	if (network_data->transproxy_enabled) {
		transproxy_onoff(FALSE, NULL);
	}
	if (network_data->tor_pid != 0) {
		kill(network_data->tor_pid, SIGTERM);
	}
	if (network_data->wait_for_tor_pid != 0) {
		kill(network_data->wait_for_tor_pid, SIGTERM);
	}
}

int transproxy_onoff(gboolean on, char *config)
{
	GError *error = NULL;
	int exit_code = 0;
	int ret = 0;

	const char *action = NULL;
	char *cmdline = NULL;

	if (on) {
		action = "enable";
		cmdline = g_strdup_printf("/usr/bin/libicd-tor-transproxy %s %s", action, config);
	} else {
		action = "disable";
		cmdline = g_strdup_printf("/usr/bin/libicd-tor-transproxy %s %s", action, "config_is_irrelevant");
	}

	g_spawn_command_line_sync(cmdline, NULL, NULL, &exit_code, &error);

	if (error) {
		ret = 1;
		TN_WARN("Failed to %s transproxy rules: %s", action, error->message);
		g_clear_error(&error);
		goto done;
	}

	if (exit_code != 0) {
		ret = 1;
		TN_WARN("%s transproxy rules failed to apply with exit code: %d", action, exit_code);
		goto done;
	}

	ret = 0;

 done:
	if (cmdline)
		g_free(cmdline);
	return ret;
}

int startup_tor(tor_network_data * network_data, char *config)
{
	char config_filename[256];
	if (snprintf(config_filename, 256, "/etc/tor/torrc-network-%s", config)
	    >= 256) {
		TN_WARN("Unable to allocate torrc config filename\n");
		return 1;
	}

	char *config_content = generate_config(config);
	GError *error = NULL;
	g_file_set_contents(config_filename, config_content, strlen(config_content), &error);
	if (error != NULL) {
		g_clear_error(&error);
		TN_WARN("Unable to write Tor config file\n");
		return 1;
	}

	char *argss[] = { "/usr/bin/tor", "-f", config_filename, NULL };
	pid_t pid = spawn_as("debian-tor", "/usr/bin/tor", argss);
	if (pid == 0) {
		TN_WARN("Failed to start Tor\n");
		return 1;
	}

	TN_INFO("Got tor_pid: %d\n", pid);
	network_data->tor_pid = pid;
	network_data->private->watch_cb(pid, network_data->private->watch_cb_token);

	network_data->transproxy_enabled = config_has_transproxy(config);

	if (network_data->transproxy_enabled) {
		transproxy_onoff(TRUE, config);
	}

	gchar *gc_controlport = g_strjoin("/", GC_TOR, config, GC_CONTROLPORT, NULL);
	GConfClient *gconf = gconf_client_get_default();
	gint control_port = gconf_client_get_int(gconf, gc_controlport, NULL);
	g_object_unref(gconf);
	g_free(gc_controlport);
	char cport[64];
	snprintf(cport, 64, "%d", control_port);

	char *argsv[] = { "/usr/bin/libicd-tor-wait-bootstrapped", cport, NULL };

	pid = spawn_as("debian-tor", "/usr/bin/libicd-tor-wait-bootstrapped", argsv);
	if (pid == 0) {
		TN_WARN("Failed to start wait for bootstrapping script\n");
		return 2;
	}
	network_data->wait_for_tor_pid = pid;
	network_data->private->watch_cb(pid, network_data->private->watch_cb_token);

	return 0;
}
