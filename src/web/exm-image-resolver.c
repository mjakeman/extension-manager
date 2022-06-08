#include "exm-image-resolver.h"

#include <libsoup/soup.h>

struct _ExmImageResolver
{
    GObject parent_instance;

    SoupSession *session;
    GMutex mutex;
};

G_DEFINE_FINAL_TYPE (ExmImageResolver, exm_image_resolver, G_TYPE_OBJECT)

enum {
    PROP_0,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmImageResolver *
exm_image_resolver_new (void)
{
    return g_object_new (EXM_TYPE_IMAGE_RESOLVER, NULL);
}

static void
exm_image_resolver_finalize (GObject *object)
{
    ExmImageResolver *self = (ExmImageResolver *)object;

    g_object_unref (self->session);
    g_mutex_clear (&self->mutex);

    G_OBJECT_CLASS (exm_image_resolver_parent_class)->finalize (object);
}

static void
image_loaded_callback (GObject      *source,
                       GAsyncResult *res,
                       GTask        *task)
{
    GBytes *bytes;
    GdkTexture *texture;

    GError *error = NULL;

    bytes = soup_session_send_and_read_finish (SOUP_SESSION (source), res, &error);

    if (error)
    {
        g_task_return_error (task, error);
    }
    else
    {
        texture = gdk_texture_new_from_bytes (bytes, &error);

        if (error)
        {
            g_task_return_error (task, error);
        }

        g_task_return_pointer (task, texture, g_object_unref);
        g_bytes_unref (bytes);
    }

    g_object_unref (task);
}

void
exm_image_resolver_resolve_async (ExmImageResolver    *self,
                                  const gchar         *rel_path,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
    GTask *task;
    const gchar *url;
    SoupMessage *msg;

    if (rel_path == NULL)
        return;

    task = g_task_new (self, cancellable, callback, user_data);

    // Resolve https://extensions.gnome.org{rel_path}

    url = g_uri_resolve_relative ("https://extensions.gnome.org/",
                                  rel_path,
                                  G_URI_FLAGS_NONE,
                                  NULL);

    msg = soup_message_new (SOUP_METHOD_GET, url);

    if (!msg)
    {
        g_task_return_new_error (task, g_quark_from_string ("exm-image-resolver"), -1,
                                 "Could not construct message for uri: %s", url);
        return;
    }

    g_mutex_lock (&self->mutex);
    soup_session_send_and_read_async (self->session, msg,
                                      G_PRIORITY_DEFAULT,
                                      cancellable,
                                      (GAsyncReadyCallback) image_loaded_callback,
                                      task);

    g_mutex_unlock (&self->mutex);
    g_object_unref (msg);
}

GdkTexture *
exm_image_resolver_resolve_finish (ExmImageResolver  *self,
                                   GAsyncResult      *result,
                                   GError           **error)
{
    g_return_val_if_fail (g_task_is_valid (result, self), NULL);

    return g_task_propagate_pointer (G_TASK (result), error);
}

static void
exm_image_resolver_class_init (ExmImageResolverClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_image_resolver_finalize;
}

static void
exm_image_resolver_init (ExmImageResolver *self)
{
    self->session = soup_session_new ();
    g_mutex_init (&self->mutex);
}
