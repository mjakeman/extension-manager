#include "exm-comment-provider.h"

#include "model/exm-comment.h"

#include <json-glib/json-glib.h>

struct _ExmCommentProvider
{
    ExmRequestHandler parent_instance;
};

G_DEFINE_FINAL_TYPE (ExmCommentProvider, exm_comment_provider, EXM_TYPE_REQUEST_HANDLER)

enum {
    PROP_0,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmCommentProvider *
exm_comment_provider_new (void)
{
    return g_object_new (EXM_TYPE_COMMENT_PROVIDER, NULL);
}

static void
exm_comment_provider_finalize (GObject *object)
{
    ExmCommentProvider *self = (ExmCommentProvider *)object;

    G_OBJECT_CLASS (exm_comment_provider_parent_class)->finalize (object);
}

static GListModel *
parse_comments (GBytes  *bytes,
                GError **out_error)
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
        // Returned format is an array of objects
        //  - each object is one comment

        GListStore *store = g_list_store_new (EXM_TYPE_COMMENT);

        JsonNode *root = json_parser_get_root (parser);
        g_assert (JSON_NODE_HOLDS_ARRAY (root));

        JsonArray *array = json_node_get_array (root);
        GList *comments = json_array_get_elements (array);

        GList *l;
        for (l = comments; l != NULL; l = l->next)
        {
            GObject *result;

            g_print ("Comment Found!\n");
            result = json_gobject_deserialize (EXM_TYPE_COMMENT, l->data);

            g_list_store_append (store, result);
        }

        return G_LIST_MODEL (store);
    }

    *out_error = error;
    return NULL;
}

void
exm_comment_provider_get_comments_async (ExmCommentProvider  *self,
                                         int                  extension_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
    // Query https://extensions.gnome.org/comments/all/?pk={%d}&all=false

    gchar *url;
    url = g_strdup_printf ("https://extensions.gnome.org/comments/all/?pk=%d&all=false", extension_id);

    exm_request_handler_request_async (EXM_REQUEST_HANDLER (self),
                                       url,
                                       cancellable,
                                       callback,
                                       user_data);
}

GListModel *
exm_comment_provider_get_comments_finish (ExmCommentProvider  *self,
                                          GAsyncResult        *result,
                                          GError             **error)
{
    gpointer ret;

    ret = exm_request_handler_request_finish (EXM_REQUEST_HANDLER (self),
                                              result,
                                              error);

    return G_LIST_MODEL (ret);
}

static void
exm_comment_provider_class_init (ExmCommentProviderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_comment_provider_finalize;

    ExmRequestHandlerClass *request_handler_class = EXM_REQUEST_HANDLER_CLASS (klass);

    request_handler_class->handle_response = (ResponseHandler) parse_comments;
}

static void
exm_comment_provider_init (ExmCommentProvider *self)
{
}
