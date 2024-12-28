/*
 * exm-versions-provider.c
 *
 * Copyright 2022-2024 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#include "model/exm-shell-version-map.h"

#include <json-glib/json-glib.h>

struct _ExmVersionsProvider
{
    ExmRequestHandler parent_instance;
};

G_DEFINE_FINAL_TYPE (ExmVersionsProvider, exm_versions_provider, EXM_TYPE_REQUEST_HANDLER)

typedef struct
{
    GListModel *list_model;
    gchar *next_page;
} VersionsRequestData;

ExmVersionsProvider *
exm_versions_provider_new (void)
{
    return g_object_new (EXM_TYPE_VERSIONS_PROVIDER, NULL);
}

static VersionsRequestData *
parse_versions_results (GBytes  *bytes,
                        GError **out_error)
{
    JsonParser *parser;
    gconstpointer data;
    gsize length;
    const gchar *next_page;
    VersionsRequestData *result;

    GError *error = NULL;
    *out_error = NULL;

    data = g_bytes_get_data (bytes, &length);

    g_debug ("Received JSON versions results:\n");
    g_debug ("%s\n", (gchar *)data);

    parser = json_parser_new ();
    if (json_parser_load_from_data (parser, data, length, &error))
    {
        // Returned format is a root object
        //  - which contains an array
        //  - of extension objects

        GListStore *store = g_list_store_new (EXM_TYPE_SHELL_VERSION_MAP);

        JsonNode *root = json_parser_get_root (parser);
        g_assert (JSON_NODE_HOLDS_OBJECT (root));

        JsonObject *root_object = json_node_get_object (root);
        g_assert (json_object_has_member (root_object, "results"));
        g_assert (json_object_has_member (root_object, "next"));

        next_page = json_object_get_string_member (root_object, "next");
        g_info ("Next page: %s\n", next_page);

        JsonArray *array = json_object_get_array_member (root_object, "results");
        GList *versions_results = json_array_get_elements (array);

        GList *l;
        for (l = versions_results; l != NULL; l = l->next)
        {
            GObject *result;

            result = json_gobject_deserialize (EXM_TYPE_SHELL_VERSION_MAP, l->data);

            g_list_store_append (store, result);
        }

        result = g_slice_new0 (VersionsRequestData);
        result->list_model = G_LIST_MODEL (store);
        result->next_page = g_strdup (next_page);
        return result;
    }

    if (out_error)
        *out_error = error;
    //if (out_num_pages)
    //    *out_num_pages = num_pages;
    return NULL;
}

void
exm_versions_provider_query_async (ExmVersionsProvider *self,
                                   const gchar         *query,
                                   int                  page,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
    // Query https://extensions.gnome.org/api/v1/extensions/{%s}/versions/

    const gchar *url;

    url = g_strdup_printf ("https://extensions.gnome.org/api/v1/extensions/%s/versions/?page=%d", query, page);

    exm_request_handler_request_async (EXM_REQUEST_HANDLER (self),
                                       url,
                                       cancellable,
                                       callback,
                                       user_data);
}

GListModel *
exm_versions_provider_query_finish (ExmVersionsProvider  *self,
                                    GAsyncResult         *result,
                                    gchar                *next_page,
                                    GError              **error)
{
    gpointer ret;
    VersionsRequestData *data;
    GListModel *list_model;

    // Check whether the task has been cancelled and if so, return null
    // This prevents a race condition in the search logic
    GCancellable *cancellable = g_task_get_cancellable (G_TASK (result));
    if (g_cancellable_is_cancelled (cancellable)) {
      return NULL;
    }

    ret = exm_request_handler_request_finish (EXM_REQUEST_HANDLER (self),
                                              result,
                                              error);

    if (ret == NULL)
        return NULL;

    data = (VersionsRequestData *) ret;

    if (next_page)
        next_page = data->next_page;

    list_model = data->list_model;

    g_slice_free (VersionsRequestData, data);

    return list_model;
}

gboolean
exm_search_result_supports_shell_version (ExmSearchResult *self,
                                          const gchar     *shell_version)
{
    g_return_val_if_fail (shell_version, FALSE);

    return exm_shell_version_map_supports (NULL/*self->shell_version_map*/,
                                           shell_version);
}

static void
deserialize_version (JsonObject         *object,
                     const gchar        *shell_version,
                     JsonNode           *member_node,
                     ExmShellVersionMap *version_map)
{
    int package;
    double version;
    JsonObject *version_obj;

    version_obj = json_node_get_object (member_node);

    package = json_object_get_int_member (version_obj, "id");
    version = json_object_get_double_member (version_obj, "version");

    exm_shell_version_map_add (version_map, shell_version, package, version);
}

static gpointer
deserialize_shell_version_map (JsonNode* node)
{
    ExmShellVersionMap *version_map;
    JsonObject *object;

    version_map = exm_shell_version_map_new ();

    g_assert (JSON_NODE_HOLDS_OBJECT (node));
    object = json_node_get_object (node);

    json_object_foreach_member (object, (JsonObjectForeach) deserialize_version, version_map);

    return version_map;
}

static void
exm_versions_provider_class_init (ExmVersionsProviderClass *klass)
{
    ExmRequestHandlerClass *request_handler_class = EXM_REQUEST_HANDLER_CLASS (klass);

    request_handler_class->handle_response = (ResponseHandler) parse_versions_results;
}

static void
exm_versions_provider_init (ExmVersionsProvider *self)
{
}
