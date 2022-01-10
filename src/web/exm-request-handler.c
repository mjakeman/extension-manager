#include "exm-request-handler.h"

#include <gio/gio.h>
#include <libsoup/soup.h>

typedef struct
{
    SoupSession *session;
} ExmRequestHandlerPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ExmRequestHandler, exm_request_handler, G_TYPE_OBJECT)

/**
 * exm_request_handler_new:
 *
 * Create a new #ExmRequestHandler.
 *
 * Returns: (transfer full): a newly created #ExmRequestHandler
 */
ExmRequestHandler *
exm_request_handler_new (void)
{
    return g_object_new (EXM_TYPE_REQUEST_HANDLER, NULL);
}

static void
exm_request_handler_finalize (GObject *object)
{
    ExmRequestHandler *self = (ExmRequestHandler *)object;
    ExmRequestHandlerPrivate *priv = exm_request_handler_get_instance_private (self);

    g_object_unref (priv->session);

    G_OBJECT_CLASS (exm_request_handler_parent_class)->finalize (object);
}

static void *
default_handle_response (GBytes  *bytes,
                         GError **out_error)
{
    *out_error = NULL;
    g_warning ("You have not overridden the `handle_response` virtual method for subclass of ExmRequestHandler. Nothing will happen.\n");
    return NULL;
}

typedef struct
{
    ExmRequestHandler *self;
    GTask *task;
} RequestData;

static void
request_callback (GObject      *source,
                  GAsyncResult *res,
                  RequestData  *data)
{
    GBytes *bytes;
    GListModel *model;

    GError *error = NULL;

    bytes = soup_session_send_and_read_finish (SOUP_SESSION (source), res, &error);

    if (error)
    {
        g_task_return_error (data->task, error);
    }
    else
    {
        // Get derived class to handle response
        ExmRequestHandlerClass *klass = EXM_REQUEST_HANDLER_CLASS (G_OBJECT_GET_CLASS (data->self));
        model = klass->handle_response (bytes, &error);

        if (model == NULL)
        {
            g_task_return_error (data->task, error);
        }
        else
        {
            g_task_return_pointer (data->task, model, g_object_unref);
        }

        g_bytes_unref (bytes);
    }

    g_object_unref (data->self);
    g_object_unref (data->task);
    g_free (data);
}

void
exm_request_handler_request_async (ExmRequestHandler   *self,
                                   const gchar         *url_endpoint,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
    ExmRequestHandlerPrivate *priv;

    GTask *task;
    SoupMessage *msg;

    priv = exm_request_handler_get_instance_private (self);

    task = g_task_new (self, cancellable, callback, user_data);
    msg = soup_message_new (SOUP_METHOD_GET, url_endpoint);

    if (!msg)
    {
        g_task_return_new_error (task, g_quark_from_string ("exm-search-provider"), -1,
                                 "Could not construct message for uri: %s", url_endpoint);
        return;
    }

    RequestData *data = g_new0 (RequestData, 1);
    data->self = g_object_ref (self);
    data->task = g_object_ref (task);

    soup_session_send_and_read_async (priv->session, msg,
                                      G_PRIORITY_DEFAULT,
                                      cancellable,
                                      (GAsyncReadyCallback) request_callback,
                                      data);

    g_object_unref (msg);
}

gpointer
exm_request_handler_request_finish (ExmRequestHandler  *self,
                                    GAsyncResult       *result,
                                    GError            **error)
{
    g_return_val_if_fail (g_task_is_valid (result, self), NULL);

    return g_task_propagate_pointer (G_TASK (result), error);
}

static void
exm_request_handler_class_init (ExmRequestHandlerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_request_handler_finalize;

    ExmRequestHandlerClass *request_handler_class = EXM_REQUEST_HANDLER_CLASS (klass);

    request_handler_class->handle_response = default_handle_response;
}

static void
exm_request_handler_init (ExmRequestHandler *self)
{
    ExmRequestHandlerPrivate *priv = exm_request_handler_get_instance_private (self);
    priv->session = soup_session_new ();
}
