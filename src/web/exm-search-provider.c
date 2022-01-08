#include "exm-search-provider.h"

#include "exm-search-result.h"

#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

struct _ExmSearchProvider
{
    GObject parent_instance;

    SoupSession *session;
};

G_DEFINE_FINAL_TYPE (ExmSearchProvider, exm_search_provider, G_TYPE_OBJECT)

enum {
    PROP_0,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmSearchProvider *
exm_search_provider_new (void)
{
    return g_object_new (EXM_TYPE_SEARCH_PROVIDER, NULL);
}

static void
exm_search_provider_finalize (GObject *object)
{
    ExmSearchProvider *self = (ExmSearchProvider *)object;

    g_object_unref (self->session);

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
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      }
}

static GListModel *
parse_search_results (ExmSearchProvider  *self,
                      GBytes             *bytes,
                      GError            **out_error)
{
    JsonParser *parser;
    gconstpointer data;
    gsize length;

    GError *error = NULL;
    *out_error = NULL;

    data = g_bytes_get_data (bytes, &length);

    g_print ("%s\n", (gchar *)data);

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

            g_print ("Extension Found!\n");
            result = json_gobject_deserialize (EXM_TYPE_SEARCH_RESULT, l->data);

            g_list_store_append (store, result);
        }

        return G_LIST_MODEL (store);
    }

    *out_error = error;
    return NULL;
}

static void
do_query_thread (GTask             *task,
                 ExmSearchProvider *self,
                 const char        *query,
                 GCancellable      *cancellable)
{
    // Query https://extensions.gnome.org/extension-query/?search={%s}

    const gchar *url;
    SoupMessage *msg;
    GBytes *bytes;

    GError *error = NULL;

    url = g_strdup_printf ("https://extensions.gnome.org/extension-query/?search=%s", query);
    msg = soup_message_new (SOUP_METHOD_GET, url);

    bytes = soup_session_send_and_read (self->session, msg, cancellable, &error);

    if (error)
    {
        g_task_return_error (task, error);
    }
    else
    {
        // Parse Search Results
        GListModel *model = parse_search_results (self, bytes, &error);

        if (model == NULL)
        {
            g_task_return_error (task, error);
        }
        else
        {
            g_task_return_pointer (task, model, g_object_unref);
        }

        g_bytes_unref (bytes);
    }

    g_object_unref (msg);
}

void
exm_search_provider_query_async (ExmSearchProvider   *self,
                                 const gchar         *query,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data)
{
    GTask *task;

    task = g_task_new (self, cancellable, callback, user_data);
    g_task_set_task_data (task, g_strdup (query), (GDestroyNotify) g_free);
    g_task_run_in_thread (task, (GTaskThreadFunc)do_query_thread);
    g_object_unref (task);
}

GListModel *
exm_search_provider_query_finish (ExmSearchProvider  *self,
                                  GAsyncResult       *result,
                                  GError            **error)
{
    g_return_val_if_fail (g_task_is_valid (result, self), NULL);

    return g_task_propagate_pointer (G_TASK (result), error);
}

static void
exm_search_provider_class_init (ExmSearchProviderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_search_provider_finalize;
    object_class->get_property = exm_search_provider_get_property;
    object_class->set_property = exm_search_provider_set_property;
}

static void
exm_search_provider_init (ExmSearchProvider *self)
{
    self->session = soup_session_new ();
}
