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
    const gchar *shell_version;
    gboolean show_unsupported;
};

G_DEFINE_FINAL_TYPE (ExmSearchProvider, exm_search_provider, EXM_TYPE_REQUEST_HANDLER)

enum {
    PROP_0,
    PROP_SHOW_UNSUPPORTED,
    PROP_SHELL_VERSION,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

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

static void
exm_search_provider_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
    ExmSearchProvider *self = EXM_SEARCH_PROVIDER (object);

    switch (prop_id)
    {
    case PROP_SHOW_UNSUPPORTED:
        g_value_set_boolean (value, self->show_unsupported);
        break;
    case PROP_SHELL_VERSION:
        g_value_set_string (value, self->shell_version);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_search_provider_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
    ExmSearchProvider *self = EXM_SEARCH_PROVIDER (object);

    switch (prop_id)
    {
    case PROP_SHOW_UNSUPPORTED:
        self->show_unsupported = g_value_get_boolean (value);
        break;
    case PROP_SHELL_VERSION:
        self->shell_version = g_value_get_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static SearchRequestData *
parse_search_results (GBytes  *bytes,
                      GError **out_error)
{
    JsonParser *parser;
    gconstpointer data;
    gsize length;
    int num_pages;
    SearchRequestData *result;

    GError *error = NULL;
    *out_error = NULL;
    num_pages = 0;

    data = g_bytes_get_data (bytes, &length);

    g_debug ("Received JSON search results:\n");
    g_debug ("%s\n", (gchar *)data);

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
        g_assert (json_object_has_member (root_object, "extensions"));
        g_assert (json_object_has_member (root_object, "numpages"));

        num_pages = json_object_get_int_member (root_object, "numpages");
        g_info ("Num Pages: %d\n", num_pages);

        JsonArray *array = json_object_get_array_member (root_object, "extensions");
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
        result->num_pages = num_pages;
        return result;
    }

    if (out_error)
        *out_error = error;
    //if (out_num_pages)
    //    *out_num_pages = num_pages;
    return NULL;
}

const gchar *
get_sort_string (ExmSearchSort sort_type)
{
    switch (sort_type)
    {
    case EXM_SEARCH_SORT_DOWNLOADS:
        return "downloads";
    case EXM_SEARCH_SORT_RECENT:
        return "created";
    case EXM_SEARCH_SORT_NAME:
        return "name";
    case EXM_SEARCH_SORT_RELEVANCE:
    default:
        return "relevance";
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
    // Query https://extensions.gnome.org/extension-query/?search={%s}&sort={%s}

    const gchar *url;
    const gchar *sort;

    sort = get_sort_string (sort_type);

    if (self->show_unsupported)
        url = g_strdup_printf ("https://extensions.gnome.org/extension-query/?search=%s&sort=%s&page=%d", query, sort, page);
    else
        url = g_strdup_printf ("https://extensions.gnome.org/extension-query/?search=%s&sort=%s&shell_version=%s&page=%d", query, sort, self->shell_version, page);

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
    if (g_cancellable_is_cancelled (cancellable)) {
      return NULL;
    }


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
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = exm_search_provider_get_property;
    object_class->set_property = exm_search_provider_set_property;

    ExmRequestHandlerClass *request_handler_class = EXM_REQUEST_HANDLER_CLASS (klass);

    request_handler_class->handle_response = (ResponseHandler) parse_search_results;

    properties [PROP_SHOW_UNSUPPORTED]
        = g_param_spec_boolean ("show-unsupported",
                                "Show Unsupported",
                                "Show Unsupported",
                                FALSE, G_PARAM_READWRITE);

    properties [PROP_SHELL_VERSION]
        = g_param_spec_string ("shell-version",
                               "Shell Version",
                               "Shell Version",
                               NULL,
                               G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_search_provider_init (ExmSearchProvider *self)
{
    // TODO: Get current GNOME Shell Version
    self->shell_version = "42";
}
