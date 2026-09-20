/*
 * exm-profile-provider.c
 *
 * Copyright 2022-2026 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#include "exm-profile-provider.h"

#include <json-glib/json-glib.h>

struct _ExmProfileProvider
{
    ExmRequestHandler parent_instance;
};

G_DEFINE_FINAL_TYPE (ExmProfileProvider, exm_profile_provider, EXM_TYPE_REQUEST_HANDLER)

ExmProfileProvider *
exm_profile_provider_new (void)
{
    return g_object_new (EXM_TYPE_PROFILE_PROVIDER, NULL);
}

static ExmUserProfile *
parse_profile (GBytes  *bytes,
               GError **out_error)
{
    JsonParser *parser;
    gconstpointer data;
    gsize length;

    GError *error = NULL;
    *out_error = NULL;

    data = g_bytes_get_data (bytes, &length);

    g_debug ("Received JSON profile:\n");
    g_debug ("%s\n", (gchar *)data);

    parser = json_parser_new ();
    if (json_parser_load_from_data (parser, data, length, &error))
    {
        JsonNode *root = json_parser_get_root (parser);
        g_assert (JSON_NODE_HOLDS_OBJECT (root));

        GObject *result = json_gobject_deserialize (EXM_TYPE_USER_PROFILE, root);

        return EXM_USER_PROFILE (result);
    }

    *out_error = error;
    return NULL;
}

void
exm_profile_provider_get_async (ExmProfileProvider  *self,
                                guint                user_id,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
    // Query https://extensions.gnome.org/api/v1/profile/{%u}/

    g_autofree gchar *url = NULL;

    url = g_strdup_printf ("https://extensions.gnome.org/api/v1/profile/%u/", user_id);

    exm_request_handler_request_async (EXM_REQUEST_HANDLER (self),
                                       url,
                                       cancellable,
                                       callback,
                                       user_data);
}

ExmUserProfile *
exm_profile_provider_get_finish (ExmProfileProvider  *self,
                                 GAsyncResult        *result,
                                 GError             **error)
{
    gpointer ret;

    ret = exm_request_handler_request_finish (EXM_REQUEST_HANDLER (self),
                                              result,
                                              error);

    return EXM_USER_PROFILE (ret);
}

static void
exm_profile_provider_class_init (ExmProfileProviderClass *klass)
{
    ExmRequestHandlerClass *request_handler_class = EXM_REQUEST_HANDLER_CLASS (klass);

    request_handler_class->handle_response = (ResponseHandler) parse_profile;

    request_handler_class->cache_ttl_seconds = 3600;
}

static void
exm_profile_provider_init (ExmProfileProvider *self G_GNUC_UNUSED)
{
}
