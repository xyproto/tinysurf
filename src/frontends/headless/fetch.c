/*
 * Copyright 2014 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of TinySurf, https://github.com/xyproto/tinysurf
 *
 * TinySurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * TinySurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <gio/gio.h>

#include "utils/errors.h"
#include "utils/file.h"
#include "utils/nsurl.h"
#include "utils/filepath.h"
#include "tinysurf/fetch.h"

#include "headless/filetype.h"
#include "headless/fetch.h"

extern char **respaths;

nserror headless_resource_data(const char *path, const uint8_t **data,
			       size_t *data_len)
{
	const gchar *const *languages = g_get_language_names();
	GBytes *bytes = NULL;
	const void *contents;
	gsize length;
	char *name;

	for (size_t i = 0; languages[i] != NULL && bytes == NULL; i++) {
		name = g_strdup_printf("/org/tinysurf/%s/%s", languages[i], path);
		bytes = g_resources_lookup_data(name, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
		g_free(name);
	}
	if (bytes == NULL) {
		name = g_strdup_printf("/org/tinysurf/%s", path);
		bytes = g_resources_lookup_data(name, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
		g_free(name);
	}
	if (bytes == NULL) {
		return NSERROR_NOT_FOUND;
	}
	contents = g_bytes_get_data(bytes, &length);
	*data = malloc(length == 0 ? 1 : length);
	if (*data == NULL) {
		g_bytes_unref(bytes);
		return NSERROR_NOMEM;
	}
	memcpy((void *)*data, contents, length);
	*data_len = length;
	g_bytes_unref(bytes);
	return NSERROR_OK;
}

static nserror headless_release_resource_data(const uint8_t *data)
{
	free((void *)data);
	return NSERROR_OK;
}

static nserror gui_get_resource_data(const char *path, const uint8_t **data,
				    size_t *data_len)
{
	char *file = filepath_find(respaths, path);

	if (file != NULL) {
		free(file);
		return NSERROR_NOT_FOUND;
	}
	return headless_resource_data(path, data, data_len);
}

static nsurl *gui_get_resource_url(const char *path)
{
	char buf[PATH_MAX];
	nsurl *url = NULL;

	tinysurf_path_to_nsurl(filepath_sfind(respaths, buf, path), &url);

	return url;
}

static struct gui_fetch_table fetch_table = {
	.filetype = headless_fetch_filetype,

	.get_resource_url = gui_get_resource_url,
	.get_resource_data = gui_get_resource_data,
	.release_resource_data = headless_release_resource_data,
};

struct gui_fetch_table *headless_fetch_table = &fetch_table;
