#include "exm-search-provider.h"

#include "model/exm-search-result.h"

#include <json-glib/json-glib.h>

struct _ExmSearchProvider
{
    ExmRequestHandler parent_instance;
};

G_DEFINE_FINAL_TYPE (ExmSearchProvider, exm_search_provider, EXM_TYPE_REQUEST_HANDLER)

ExmSearchProvider *
exm_search_provider_new (void)
{
    return g_object_new (EXM_TYPE_SEARCH_PROVIDER, NULL);
}

static void
exm_search_provider_finalize (GObject *object)
{
    ExmSearchProvider *self = (ExmSearchProvider *)object;

    G_OBJECT_CLASS (exm_search_provider_parent_class)->finalize (object);
}

static GListModel *
parse_search_results (GBytes  *bytes,
                      GError **out_error)
{
    JsonParser *parser;
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
        // Returned format is a root object
        //  - which contains an array
        //  - of extension objects

        GListStore *store = g_list_store_new (EXM_TYPE_SEARCH_RESULT);

        JsonNode *root = json_parser_get_root (parser);
        g_assert (JSON_NODE_HOLDS_OBJECT (root));

        JsonObject *root_object = json_node_get_object (root);
        g_assert (json_object_has_member (root_object, "extensions"));

        JsonArray *array = json_object_get_array_member (root_object, "extensions");
        GList *search_results = json_array_get_elements (array);

        GList *l;
        for (l = search_results; l != NULL; l = l->next)
        {
            GObject *result;

            result = json_gobject_deserialize (EXM_TYPE_SEARCH_RESULT, l->data);

            g_list_store_append (store, result);
        }

        return G_LIST_MODEL (store);
    }

    *out_error = error;
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
        return "recent";
    case EXM_SEARCH_SORT_NAME:
        return "name";
    case EXM_SEARCH_SORT_POPULARITY:
    default:
        return "popularity";
    }
}

void
exm_search_provider_query_async (ExmSearchProvider   *self,
                                 const gchar         *query,
                                 ExmSearchSort        sort_type,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data)
{
    // Query https://extensions.gnome.org/extension-query/?search={%s}&sort={%s}

    const gchar *url;
    const gchar *sort;

    sort = get_sort_string (sort_type);
    url = g_strdup_printf ("https://extensions.gnome.org/extension-query/?search=%s&sort=%s", query, sort);

    exm_request_handler_request_async (EXM_REQUEST_HANDLER (self),
                                       url,
                                       cancellable,
                                       callback,
                                       user_data);
}

GListModel *
exm_search_provider_query_finish (ExmSearchProvider  *self,
                                  GAsyncResult       *result,
                                  GError            **error)
{
    gpointer ret;

    ret = exm_request_handler_request_finish (EXM_REQUEST_HANDLER (self),
                                              result,
                                              error);

    return G_LIST_MODEL (ret);
}

static void
exm_search_provider_class_init (ExmSearchProviderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_search_provider_finalize;

    ExmRequestHandlerClass *request_handler_class = EXM_REQUEST_HANDLER_CLASS (klass);

    request_handler_class->handle_response = (ResponseHandler) parse_search_results;
}

static void
exm_search_provider_init (ExmSearchProvider *self)
{
}
