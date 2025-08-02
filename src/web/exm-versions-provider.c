/*
 * exm-versions-provider.c
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

#include "exm-versions-provider.h"

#include "model/exm-version-result.h"

#include <json-glib/json-glib.h>

struct _ExmVersionsProvider
{
    ExmRequestHandler parent_instance;
    const gchar *shell_version;
};

G_DEFINE_FINAL_TYPE (ExmVersionsProvider, exm_versions_provider, EXM_TYPE_REQUEST_HANDLER)

enum {
    PROP_0,
    PROP_SHELL_VERSION,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

typedef struct
{
    GListModel *list_model;
    gchar *next;
} VersionsRequestData;

ExmVersionsProvider *
exm_versions_provider_new (void)
{
    return g_object_new (EXM_TYPE_VERSIONS_PROVIDER, NULL);
}

static void
exm_versions_provider_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
    ExmVersionsProvider *self = EXM_VERSIONS_PROVIDER (object);

    switch (prop_id)
    {
    case PROP_SHELL_VERSION:
        g_value_set_string (value, self->shell_version);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_versions_provider_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
    ExmVersionsProvider *self = EXM_VERSIONS_PROVIDER (object);

    switch (prop_id)
    {
    case PROP_SHELL_VERSION:
        self->shell_version = g_value_get_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static VersionsRequestData *
parse_versions_results (GBytes  *bytes,
                        GError **out_error)
{
    JsonParser *parser;
    const gchar *data;
    gsize length;
    VersionsRequestData *result;
    GError *error = NULL;
    *out_error = NULL;

    data = g_bytes_get_data (bytes, &length);

    g_debug ("Received JSON versions results:\n");
    g_debug ("%s\n", data);

    parser = json_parser_new ();
    if (json_parser_load_from_data (parser, data, length, &error))
    {
        // Returned format is a root object
        //  - which contains an array
        //  - of version objects

        GListStore *store = g_list_store_new (EXM_TYPE_VERSION_RESULT);

        JsonNode *root = json_parser_get_root (parser);
        g_assert (JSON_NODE_HOLDS_OBJECT (root));

        JsonObject *root_object = json_node_get_object (root);
        g_assert (json_object_has_member (root_object, "results"));

        JsonArray *array = json_object_get_array_member (root_object, "results");
        GList *versions_results = json_array_get_elements (array);

        GList *l;
        for (l = versions_results; l != NULL; l = l->next)
        {
            GObject *result;

            result = json_gobject_deserialize (EXM_TYPE_VERSION_RESULT, l->data);

            g_list_store_append (store, result);
        }

        const gchar *next = json_object_get_string_member_with_default (root_object, "next", NULL);

        result = g_slice_new0 (VersionsRequestData);
        result->list_model = G_LIST_MODEL (store);
        result->next = g_strdup (next);
        return result;
    }

    if (out_error)
        *out_error = error;

    return NULL;
}

void
exm_versions_provider_query_async (ExmVersionsProvider *self,
                                   const gchar         *uuid,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
    // Query https://extensions.gnome.org/api/v1/extensions/{%s}/versions/

    const gchar *url;

    url = g_strdup_printf ("https://extensions.gnome.org/api/v1/extensions/%s/versions/?page=1&page_size=100", uuid);

    exm_request_handler_request_async (EXM_REQUEST_HANDLER (self),
                                       url,
                                       cancellable,
                                       callback,
                                       user_data);
}

void
exm_versions_provider_query_next_async (ExmVersionsProvider *self,
                                        gchar               *url,
                                        GCancellable        *cancellable,
                                        GAsyncReadyCallback  callback,
                                        gpointer             user_data)
{
    exm_request_handler_request_async (EXM_REQUEST_HANDLER (self),
                                       url,
                                       cancellable,
                                       callback,
                                       user_data);
}

GListModel *
exm_versions_provider_query_finish (ExmVersionsProvider  *self,
                                    GAsyncResult         *result,
                                    gchar               **next,
                                    GError              **error)
{
    gpointer ret;
    VersionsRequestData *data;
    GListModel *list_model;

    // Check whether the task has been cancelled and if so, return null
    // This prevents a race condition in the search logic
    GCancellable *cancellable = g_task_get_cancellable (G_TASK (result));
    if (g_cancellable_is_cancelled (cancellable))
        return NULL;

    ret = exm_request_handler_request_finish (EXM_REQUEST_HANDLER (self),
                                              result,
                                              error);

    if (ret == NULL)
        return NULL;

    data = (VersionsRequestData *) ret;

    if (next != NULL)
        *next = g_strdup (data->next);

    list_model = data->list_model;

    g_slice_free (VersionsRequestData, data);

    return list_model;
}

static void
exm_versions_provider_class_init (ExmVersionsProviderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = exm_versions_provider_get_property;
    object_class->set_property = exm_versions_provider_set_property;

    ExmRequestHandlerClass *request_handler_class = EXM_REQUEST_HANDLER_CLASS (klass);

    request_handler_class->handle_response = (ResponseHandler) parse_versions_results;

    properties [PROP_SHELL_VERSION]
        = g_param_spec_string ("shell-version",
                               "Shell Version",
                               "Shell Version",
                               NULL,
                               G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_versions_provider_init (ExmVersionsProvider *self)
{
    // TODO: Get current GNOME Shell Version
    self->shell_version = "42";
}
