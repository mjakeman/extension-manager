/*
 * exm-data-provider.c
 *
 * Copyright 2022-2025 Matthew Jakeman <mjakeman26@outlook.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "exm-data-provider.h"

#include <json-glib/json-glib.h>

struct _ExmDataProvider
{
    ExmRequestHandler parent_instance;
};

G_DEFINE_FINAL_TYPE (ExmDataProvider, exm_data_provider, EXM_TYPE_REQUEST_HANDLER)

ExmDataProvider *
exm_data_provider_new (void)
{
    return g_object_new (EXM_TYPE_DATA_PROVIDER, NULL);
}

static ExmSearchResult *
parse_extension (GBytes  *bytes,
                 GError **out_error)
{
    g_autoptr (JsonParser) parser = NULL;
    gconstpointer data;
    gsize length;

    GError *error = NULL;
    *out_error = NULL;

    data = g_bytes_get_data (bytes, &length);

    g_debug ("Received JSON search results:\n");
    g_debug ("%s\n", (gchar *)data);

    parser = json_parser_new ();
    if (json_parser_load_from_data (parser, data, length, &error))
    {
        // Returned format is one single extension object
        JsonNode *root = json_parser_get_root (parser);

        if (root == NULL || !JSON_NODE_HOLDS_OBJECT (root))
        {
            g_set_error (out_error, g_quark_from_string ("exm-data-provider"), -1,
                        "Unexpected response format from extensions.gnome.org");
            return NULL;
        }

        GObject *result = json_gobject_deserialize (EXM_TYPE_SEARCH_RESULT, root);

        return EXM_SEARCH_RESULT (result);
    }

    *out_error = error;
    return NULL;
}

void
exm_data_provider_get_async (ExmDataProvider     *self,
                             const gchar         *uuid,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data)
{
    // Query https://extensions.gnome.org/api/v1/extensions/{%s}/

    g_autofree gchar *escaped_uuid = g_uri_escape_string (uuid, NULL, FALSE);
    g_autofree gchar *url = NULL;

    url = g_strdup_printf ("https://extensions.gnome.org/api/v1/extensions/%s/", escaped_uuid);

    exm_request_handler_request_async (EXM_REQUEST_HANDLER (self),
                                       url,
                                       cancellable,
                                       callback,
                                       user_data);
}

ExmSearchResult *
exm_data_provider_get_finish (ExmDataProvider  *self,
                              GAsyncResult     *result,
                              GError          **error)
{
    gpointer ret;

    ret = exm_request_handler_request_finish (EXM_REQUEST_HANDLER (self),
                                              result,
                                              error);

    return EXM_SEARCH_RESULT (ret);
}

static void
exm_data_provider_class_init (ExmDataProviderClass *klass)
{
    ExmRequestHandlerClass *request_handler_class = EXM_REQUEST_HANDLER_CLASS (klass);

    request_handler_class->handle_response = (ResponseHandler) parse_extension;

    request_handler_class->cache_ttl_seconds = 3600;
}

static void
exm_data_provider_init (ExmDataProvider *self G_GNUC_UNUSED)
{
}
