/*
 * exm-search-provider.c
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

#include "exm-search-provider.h"

#include "model/exm-search-result.h"

#include <json-glib/json-glib.h>

struct _ExmSearchProvider
{
    ExmRequestHandler parent_instance;
};

G_DEFINE_FINAL_TYPE (ExmSearchProvider, exm_search_provider, EXM_TYPE_REQUEST_HANDLER)

typedef struct
{
    GListModel *list_model;
    int num_pages;
} SearchRequestData;

ExmSearchProvider *
exm_search_provider_new (void)
{
    return g_object_new (EXM_TYPE_SEARCH_PROVIDER, NULL);
}

static SearchRequestData *
parse_search_results (GBytes  *bytes,
                      GError **out_error)
{
    JsonParser *parser;
    const gchar *data;
    gsize length;
    int count;
    SearchRequestData *result;

    GError *error = NULL;
    *out_error = NULL;

    data = g_bytes_get_data (bytes, &length);

    g_debug ("Received JSON search results:\n");
    g_debug ("%s\n", data);

    parser = json_parser_new ();
    if (json_parser_load_from_data (parser, data, length, &error))
    {
        // Returned format is a root object
        //  - which contains an array
        //  - of extension objects

        GListStore *store = g_list_store_new (EXM_TYPE_SEARCH_RESULT);

        JsonNode *root = json_parser_get_root (parser);
        g_assert (JSON_NODE_HOLDS_OBJECT (root));

        JsonObject *root_object = json_node_get_object (root);
        g_assert (json_object_has_member (root_object, "count"));
        g_assert (json_object_has_member (root_object, "results"));

        count = json_object_get_int_member (root_object, "count");
        g_info ("Count: %d\n", count);

        JsonArray *array = json_object_get_array_member (root_object, "results");
        GList *search_results = json_array_get_elements (array);

        GList *l;
        for (l = search_results; l != NULL; l = l->next)
        {
            GObject *result;

            result = json_gobject_deserialize (EXM_TYPE_SEARCH_RESULT, l->data);

            g_list_store_append (store, result);
        }

        result = g_slice_new0 (SearchRequestData);
        result->list_model = G_LIST_MODEL (store);
        result->num_pages = (count + 10 - 1) / 10;
        return result;
    }

    if (out_error)
        *out_error = error;

    return NULL;
}

const gchar *
get_sort_string (ExmSearchSort sort_type)
{
    switch (sort_type)
    {
    case EXM_SEARCH_SORT_CREATED_DES:
        return "-created";
    case EXM_SEARCH_SORT_CREATED_ASC:
        return "created";
    case EXM_SEARCH_SORT_DOWNLOADS_DES:
        return "-downloads";
    case EXM_SEARCH_SORT_DOWNLOADS_ASC:
        return "downloads";
    case EXM_SEARCH_SORT_UPDATED_DES:
        return "-updated";
    case EXM_SEARCH_SORT_UPDATED_ASC:
        return "updated";
    case EXM_SEARCH_SORT_POPULARITY_DES:
        return "-popularity";
    case EXM_SEARCH_SORT_POPULARITY_ASC:
    default:
        return "popularity";
    }
}

void
exm_search_provider_query_async (ExmSearchProvider   *self,
                                 const gchar         *query,
                                 int                  page,
                                 ExmSearchSort        sort_type,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data)
{
    // Query https://extensions.gnome.org/api/v1/extensions/?ordering={%s}&page={%d}&status=3
    // Query https://extensions.gnome.org/api/v1/extensions/search/{%s}/?ordering={%s}&page={%d}

    const gchar *url;
    const gchar *sort;

    sort = get_sort_string (sort_type);

    if (g_strcmp0 (query, "") == 0)
        url = g_strdup_printf ("https://extensions.gnome.org/api/v1/extensions/?ordering=%s&page=%d&page_size=10&status=3", sort, page);
    else
        url = g_strdup_printf ("https://extensions.gnome.org/api/v1/extensions/search/%s/?ordering=%s&page=%d&page_size=10", query, sort, page);

    exm_request_handler_request_async (EXM_REQUEST_HANDLER (self),
                                       url,
                                       cancellable,
                                       callback,
                                       user_data);
}

GListModel *
exm_search_provider_query_finish (ExmSearchProvider  *self,
                                  GAsyncResult       *result,
                                  int                *num_pages,
                                  GError            **error)
{
    gpointer ret;
    SearchRequestData *data;
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

    data = (SearchRequestData *) ret;

    if (num_pages)
        *num_pages = data->num_pages;

    list_model = data->list_model;

    g_slice_free (SearchRequestData, data);

    return list_model;
}

static void
exm_search_provider_class_init (ExmSearchProviderClass *klass)
{
    ExmRequestHandlerClass *request_handler_class = EXM_REQUEST_HANDLER_CLASS (klass);

    request_handler_class->handle_response = (ResponseHandler) parse_search_results;
}

static void
exm_search_provider_init (ExmSearchProvider *self G_GNUC_UNUSED)
{
}
