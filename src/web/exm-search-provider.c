#include "exm-search-provider.h"

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

static void
parse_search_results (ExmSearchProvider *self,
                      GBytes            *bytes)
{
    JsonParser *parser;
    gconstpointer data;
    gsize length;

    GError *error = NULL;

    data = g_bytes_get_data (bytes, &length);

    g_print ("%s\n", (gchar *)data);

    // TODO: Look at making this asynchronous?
    parser = json_parser_new ();
    if (json_parser_load_from_data (parser, data, length, &error))
    {

    }

    if (error)
    {
        g_printerr ("Could not parse JSON search results: %s\n", error->message);
        g_error_free (error);
    }

    return;
}

static void
search_result_cb (GObject           *source,
                  GAsyncResult      *result,
                  ExmSearchProvider *self)
{
    GError *error = NULL;
    GBytes *bytes = soup_session_send_and_read_finish (SOUP_SESSION (source), result, &error);

    // Usage here is the same as before
    if (error) {
        g_error_free (error);
    } else {
        parse_search_results (self, bytes);
        g_bytes_unref (bytes);
    }
}

void
exm_search_provider_query (ExmSearchProvider *self,
                           const gchar       *query)
{
    // Query https://extensions.gnome.org/extension-query/?search={%s}

    const gchar *url;
    SoupMessage *msg;

    url = g_strdup_printf ("https://extensions.gnome.org/extension-query/?search=%s", query);
    msg = soup_message_new (SOUP_METHOD_GET, url);

    soup_session_send_and_read_async (
        self->session,
        msg,
        G_PRIORITY_DEFAULT,
        NULL,
        (GAsyncReadyCallback) search_result_cb,
        self);

    g_object_unref (msg);
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
