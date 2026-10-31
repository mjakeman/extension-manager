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

    gchar *shell_version;
};

G_DEFINE_FINAL_TYPE (ExmSearchProvider, exm_search_provider, EXM_TYPE_REQUEST_HANDLER)

enum {
    PROP_0,
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

static gboolean
is_number (const gchar *string)
{
    if (!string || !*string)
        return FALSE;

    for (const gchar *c = string; *c; c++)
    {
        if (!g_ascii_isdigit (*c))
            return FALSE;
    }

    return TRUE;
}

static gchar *
normalize_shell_version (const gchar *shell_version)
{
    g_auto (GStrv) parts = NULL;

    if (!shell_version)
        return NULL;

    parts = g_strsplit (shell_version, ".", 3);

    if (!is_number (parts[0]))
        return NULL;

    if (is_number (parts[1]) ||
        g_strcmp0 (parts[1], "alpha") == 0 ||
        g_strcmp0 (parts[1], "beta") == 0 ||
        g_strcmp0 (parts[1], "rc") == 0)
        return g_strdup_printf ("%s.%s", parts[0], parts[1]);

    return g_strdup (parts[0]);
}

static SearchRequestData *
parse_search_results (GBytes  *bytes,
                      GError **out_error)
{
    g_autoptr (JsonParser) parser = NULL;
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

        if (root == NULL || !JSON_NODE_HOLDS_OBJECT (root))
        {
            g_object_unref (store);
            g_set_error (out_error, g_quark_from_string ("exm-search-provider"), -1,
                        "Unexpected response format from extensions.gnome.org");
            return NULL;
        }

        JsonObject *root_object = json_node_get_object (root);

        if (!json_object_has_member (root_object, "count") ||
            !json_object_has_member (root_object, "results"))
        {
            g_object_unref (store);
            g_set_error (out_error, g_quark_from_string ("exm-search-provider"), -1,
                        "Unexpected response format from extensions.gnome.org");
            return NULL;
        }

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
        g_list_free (search_results);

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
    case EXM_SEARCH_SORT_POPULARITY_DES:
        return "-popularity";
    case EXM_SEARCH_SORT_POPULARITY_ASC:
        return "popularity";
    case EXM_SEARCH_SORT_UPDATED_DES:
        return "-updated";
    case EXM_SEARCH_SORT_UPDATED_ASC:
        return "updated";
    case EXM_SEARCH_SORT_NONE:
    default:
        return "";
    }
}

void
exm_search_provider_query_async (ExmSearchProvider   *self,
                                 const gchar         *query,
                                 int                  page,
                                 int                  page_size,
                                 ExmSearchSort        sort_type,
                                 gboolean             only_supported,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data)
{
    // Query https://extensions.gnome.org/api/v1/extensions/?ordering={%s}&page={%d}&status=3
    // Query https://extensions.gnome.org/api/v1/extensions/search/{%s}/?ordering={%s}&page={%d}

    g_autofree gchar *url = NULL;
    g_autofree gchar *sort = NULL;
    g_autofree gchar *version = NULL;
    const gchar *sort_str;

    sort_str = get_sort_string (sort_type);

    if (g_strcmp0 (sort_str, "") != 0)
        sort = g_strdup_printf ("ordering=%s&", sort_str);

    if (only_supported)
    {
        g_autofree gchar *normalized_version = normalize_shell_version (self->shell_version);

        if (normalized_version)
            version = g_strdup_printf ("&shell_version=%s", normalized_version);
    }

    if (g_strcmp0 (query, "") == 0)
    {
        if (sort == NULL)
            sort = g_strdup_printf ("ordering=-popularity&");

        url = g_strdup_printf ("https://extensions.gnome.org/api/v1/extensions/?%spage=%d&page_size=%d&status=3%s",
                               sort, page, page_size, version ? version : "");
    }
    else
    {
        g_autofree gchar *escaped_query = g_uri_escape_string (query, NULL, FALSE);

        url = g_strdup_printf ("https://extensions.gnome.org/api/v1/extensions/search/%s/?%spage=%d&page_size=%d%s",
                               escaped_query, sort ? sort : "", page, page_size, version ? version : "");
    }

    exm_request_handler_request_async (EXM_REQUEST_HANDLER (self),
                                       url,
                                       cancellable,
                                       callback,
                                       user_data);
}

gboolean
exm_search_provider_query_uuids_async (ExmSearchProvider   *self,
                                       const gchar * const *uuids,
                                       guint                n_uuids,
                                       const gchar         *shell_version,
                                       GCancellable        *cancellable,
                                       GAsyncReadyCallback  callback,
                                       gpointer             user_data)
{
    // Query https://extensions.gnome.org/api/v1/extensions/?uuid={%s}&uuid={%s}&shell_version={%s}&status=3

    g_autoptr (GString) url = g_string_new (NULL);
    guint n_valid = 0;

    g_return_val_if_fail (n_uuids <= EXM_SEARCH_PROVIDER_MAX_UUIDS_PER_REQUEST, FALSE);

    g_string_append_printf (url, "https://extensions.gnome.org/api/v1/extensions/?page_size=%u",
                            n_uuids);

    if (shell_version)
    {
        g_autofree gchar *normalized_version = normalize_shell_version (shell_version);

        if (!normalized_version)
            return FALSE;

        g_string_append_printf (url, "&status=3&shell_version=%s", normalized_version);
    }

    for (guint i = 0; i < n_uuids; i++)
    {
        g_autofree gchar *escaped_uuid = NULL;

        if (!uuids[i])
            continue;

        escaped_uuid = g_uri_escape_string (uuids[i], NULL, FALSE);
        g_string_append_printf (url, "&uuid=%s", escaped_uuid);
        n_valid++;
    }

    if (n_valid == 0)
        return FALSE;

    exm_request_handler_request_async (EXM_REQUEST_HANDLER (self),
                                       url->str,
                                       cancellable,
                                       callback,
                                       user_data);

    return TRUE;
}

static gboolean
query_supported_async (ExmSearchProvider   *self,
                       GListModel          *results,
                       GCancellable        *cancellable,
                       GAsyncReadyCallback  callback,
                       gpointer             user_data)
{
    g_autoptr (GPtrArray) uuids = g_ptr_array_new_with_free_func (g_free);
    guint n_items = g_list_model_get_n_items (results);

    if (!self->shell_version)
        return FALSE;

    for (guint i = 0; i < n_items; i++)
    {
        g_autoptr (ExmSearchResult) result = g_list_model_get_item (results, i);
        gchar *uuid = NULL;

        g_object_get (result, "uuid", &uuid, NULL);

        if (uuid)
            g_ptr_array_add (uuids, uuid);
    }

    return exm_search_provider_query_uuids_async (self,
                                                  (const gchar * const *) uuids->pdata,
                                                  uuids->len,
                                                  self->shell_version,
                                                  cancellable,
                                                  callback,
                                                  user_data);
}

typedef struct
{
    ExmSearchProvider *provider;
    GListStore *results;
    gchar *shell_version;
} SupportCheck;

static void
support_check_free (SupportCheck *check)
{
    g_object_unref (check->results);
    g_object_unref (check->provider);
    g_free (check->shell_version);
    g_free (check);
}

static void
on_support_checked (GObject      *source,
                    GAsyncResult *res,
                    gpointer      user_data)
{
    SupportCheck *check = user_data;
    g_autoptr (GListModel) supported = NULL;
    g_autoptr (GError) error = NULL;

    supported = exm_search_provider_query_finish (EXM_SEARCH_PROVIDER (source), res, NULL, &error);

    if (supported)
    {
        g_autoptr (GHashTable) uuids = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
        guint n_items = g_list_model_get_n_items (supported);

        for (guint i = 0; i < n_items; i++)
        {
            g_autoptr (ExmSearchResult) result = g_list_model_get_item (supported, i);
            gchar *uuid = NULL;

            g_object_get (result, "uuid", &uuid, NULL);

            if (uuid)
                g_hash_table_add (uuids, uuid);
        }

        n_items = g_list_model_get_n_items (G_LIST_MODEL (check->results));

        for (guint i = 0; i < n_items; i++)
        {
            g_autoptr (ExmSearchResult) result = g_list_model_get_item (G_LIST_MODEL (check->results), i);
            g_autofree gchar *uuid = NULL;

            g_object_get (result, "uuid", &uuid, NULL);

            if (!uuid)
                continue;

            exm_search_result_set_unsupported (result, g_hash_table_contains (uuids, uuid) ? NULL
                                                                                          : check->shell_version);
        }
    }

    support_check_free (check);
}

void
exm_search_provider_mark_unsupported (ExmSearchProvider *self,
                                      GListModel        *results,
                                      guint              start,
                                      GCancellable      *cancellable)
{
    guint n_items = g_list_model_get_n_items (results);

    for (guint i = start; i < n_items; i += EXM_SEARCH_PROVIDER_MAX_UUIDS_PER_REQUEST)
    {
        SupportCheck *check = g_new0 (SupportCheck, 1);

        check->provider = g_object_ref (self);
        check->results = g_list_store_new (EXM_TYPE_SEARCH_RESULT);
        check->shell_version = g_strdup (self->shell_version);

        for (guint j = i; j < MIN (i + EXM_SEARCH_PROVIDER_MAX_UUIDS_PER_REQUEST, n_items); j++)
        {
            g_autoptr (ExmSearchResult) result = g_list_model_get_item (results, j);

            g_list_store_append (check->results, result);
        }

        if (!query_supported_async (self, G_LIST_MODEL (check->results), cancellable,
                                    on_support_checked, check))
        {
            support_check_free (check);
            break;
        }
    }
}

void
exm_search_provider_query_by_creator_async (ExmSearchProvider   *self,
                                            const gchar         *username,
                                            GCancellable        *cancellable,
                                            GAsyncReadyCallback  callback,
                                            gpointer             user_data)
{
    // Query https://extensions.gnome.org/api/v1/extensions/search/{%s}/?ordering=name&page_size=100

    gchar *escaped = g_uri_escape_string (username, NULL, FALSE);
    gchar *url = g_strdup_printf ("https://extensions.gnome.org/api/v1/extensions/search/%s/?ordering=name&page_size=100",
                                  escaped);

    g_free (escaped);

    exm_request_handler_request_async (EXM_REQUEST_HANDLER (self),
                                       url,
                                       cancellable,
                                       callback,
                                       user_data);

    g_free (url);
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

    ret = exm_request_handler_request_finish (EXM_REQUEST_HANDLER (self),
                                              result,
                                              error);

    // Check whether the task has been cancelled and if so, return null
    // This prevents a race condition in the search logic
    if (g_cancellable_is_cancelled (g_task_get_cancellable (G_TASK (result))))
    {
        if (ret != NULL)
        {
            data = (SearchRequestData *) ret;
            g_object_unref (data->list_model);
            g_slice_free (SearchRequestData, data);
        }

        if (error)
            g_clear_error (error);

        return NULL;
    }

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
exm_search_provider_finalize (GObject *object)
{
    ExmSearchProvider *self = EXM_SEARCH_PROVIDER (object);

    g_clear_pointer (&self->shell_version, g_free);

    G_OBJECT_CLASS (exm_search_provider_parent_class)->finalize (object);
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
    case PROP_SHELL_VERSION:
        if (g_strcmp0 (self->shell_version, g_value_get_string (value)) != 0)
        {
            g_free (self->shell_version);
            self->shell_version = g_value_dup_string (value);
            g_object_notify_by_pspec (object, properties [PROP_SHELL_VERSION]);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_search_provider_class_init (ExmSearchProviderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    ExmRequestHandlerClass *request_handler_class = EXM_REQUEST_HANDLER_CLASS (klass);

    object_class->finalize = exm_search_provider_finalize;
    object_class->get_property = exm_search_provider_get_property;
    object_class->set_property = exm_search_provider_set_property;

    request_handler_class->handle_response = (ResponseHandler) parse_search_results;

    properties [PROP_SHELL_VERSION] =
        g_param_spec_string ("shell-version",
                             "Shell Version",
                             "Shell Version",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_search_provider_init (ExmSearchProvider *self G_GNUC_UNUSED)
{
}
