/*
 * exm-version-provider.c
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

#include "exm-version-provider.h"

#include <json-glib/json-glib.h>

struct _ExmVersionProvider
{
    ExmRequestHandler parent_instance;
};

G_DEFINE_FINAL_TYPE (ExmVersionProvider, exm_version_provider, EXM_TYPE_REQUEST_HANDLER)

ExmVersionProvider *
exm_version_provider_new (void)
{
    return g_object_new (EXM_TYPE_VERSION_PROVIDER, NULL);
}

static ExmVersionResult *
parse_version (GBytes  *bytes,
               GError **out_error)
{
    JsonParser *parser;
    const gchar *data;
    gsize length;
    GError *error = NULL;
    *out_error = NULL;

    data = g_bytes_get_data (bytes, &length);

    g_debug ("Received JSON version results:\n");
    g_debug ("%s\n", data);

    parser = json_parser_new ();
    if (json_parser_load_from_data (parser, data, length, &error))
    {
        // Returned format is one single version object
        JsonNode *root = json_parser_get_root (parser);
        g_assert (JSON_NODE_HOLDS_OBJECT (root));

        GObject *result = json_gobject_deserialize (EXM_TYPE_VERSION_RESULT, root);

        return EXM_VERSION_RESULT (result);
    }

    *out_error = error;
    return NULL;
}

void
exm_version_provider_get_async (ExmVersionProvider  *self,
                                const gchar         *uuid,
                                const gchar         *version,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
    // Query https://extensions.gnome.org/api/v1/extensions/{%s}/versions/{%s}/

    const gchar *url;

    url = g_strdup_printf ("https://extensions.gnome.org/api/v1/extensions/%s/versions/%s/", uuid, version);

    exm_request_handler_request_async (EXM_REQUEST_HANDLER (self),
                                       url,
                                       cancellable,
                                       callback,
                                       user_data);
}

ExmVersionResult *
exm_version_provider_get_finish (ExmVersionProvider  *self,
                                 GAsyncResult        *result,
                                 GError             **error)
{
    gpointer ret;

    ret = exm_request_handler_request_finish (EXM_REQUEST_HANDLER (self),
                                              result,
                                              error);

    return EXM_VERSION_RESULT (ret);
}

static void
exm_version_provider_class_init (ExmVersionProviderClass *klass)
{
    ExmRequestHandlerClass *request_handler_class = EXM_REQUEST_HANDLER_CLASS (klass);

    request_handler_class->handle_response = (ResponseHandler) parse_version;
}

static void
exm_version_provider_init (ExmVersionProvider *self G_GNUC_UNUSED)
{
}
