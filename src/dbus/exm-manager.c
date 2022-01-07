#include "exm-manager.h"

#include <gio/gio.h>

struct _ExmManager
{
    GObject parent_instance;

    GDBusProxy* proxy;
};

G_DEFINE_FINAL_TYPE (ExmManager, exm_manager, G_TYPE_OBJECT)

enum {
    PROP_0,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmManager *
exm_manager_new (void)
{
    return g_object_new (EXM_TYPE_MANAGER, NULL);
}

static void
exm_manager_finalize (GObject *object)
{
    ExmManager *self = (ExmManager *)object;

    G_OBJECT_CLASS (exm_manager_parent_class)->finalize (object);
}

static void
exm_manager_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    ExmManager *self = EXM_MANAGER (object);

    switch (prop_id)
      {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      }
}

static void
exm_manager_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    ExmManager *self = EXM_MANAGER (object);

    switch (prop_id)
      {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      }
}

static void
exm_manager_class_init (ExmManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_manager_finalize;
    object_class->get_property = exm_manager_get_property;
    object_class->set_property = exm_manager_set_property;
}

static void
parse_extension_list (GVariant *exlist)
{
    /* format: a{sa{sv}}
     * array of interfaces, where each interface is an array of properties
     * each interface corresponds to one extension
     * this is terrible >:(
     * see also: https://stackoverflow.com/questions/54131543/how-can-i-get-the-g-dbus-connection-signal-subscribe-function-to-tell-me-about-p
     */
    GVariantIter *iter, *iter2;
    gchar *exname, *prop_name;
    GVariant *prop_value;

    g_variant_get (exlist, "(a{sa{sv}})", &iter);
    while (g_variant_iter_loop(iter, "{sa{sv}}", &exname, &iter2)) {
        g_print ("Extension Discovered: %s\n", exname);

        while (g_variant_iter_loop(iter2, "{sv}", &prop_name, &prop_value))
        {
            g_print ("Property: %s=%s\n", prop_name, g_variant_print(prop_value, 0));
        }
    }
}

static void
exm_manager_init (ExmManager *self)
{
    GError *error = NULL;

    self->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL,
                                                 "org.gnome.Shell.Extensions", "/org/gnome/Shell/Extensions",
                                                 "org.gnome.Shell.Extensions", NULL, &error);

    if (error != NULL)
    {
        g_critical ("Could not create proxy: %s\n", error->message);
        return;
    }

    GVariant* exlist = g_dbus_proxy_call_sync (self->proxy, "ListExtensions", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

    if (error != NULL)
    {
        g_critical ("Could not list extensions: %s\n", error->message);
        return;
    }

    parse_extension_list (exlist);
}
