#include "exm-manager.h"

#include "exm-extension.h"
#include "shell-dbus-interface.h"

struct _ExmManager
{
    GObject parent_instance;

    ShellExtensions *proxy;
    GListModel *user_ext_model;
    GListModel *system_ext_model;
};

G_DEFINE_FINAL_TYPE (ExmManager, exm_manager, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_USER_EXTENSIONS,
    PROP_SYSTEM_EXTENSIONS,
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
    case PROP_USER_EXTENSIONS:
        g_value_set_object (value, self->user_ext_model);
        break;
    case PROP_SYSTEM_EXTENSIONS:
        g_value_set_object (value, self->system_ext_model);
        break;
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
enable_extension_done (ShellExtensions *proxy,
                       GAsyncResult    *res,
                       ExmExtension    *extension)
{
    GError *error = NULL;
    gboolean success;
    shell_extensions_call_enable_extension_finish (proxy, &success, res, &error);

    if (!success)
    {
        gchar *uuid;
        g_object_get (extension, "uuid", &uuid, NULL);
        if (error)
            g_critical ("Could not enable extension '%s': %s\n", uuid, error->message);
        else
            g_critical ("Could not enable extension '%s': unknown failure", uuid);
    }
}

void
exm_manager_enable_extension (ExmManager   *self,
                              ExmExtension *extension)
{
    gchar *uuid;
    g_object_get (extension, "uuid", &uuid, NULL);

    shell_extensions_call_enable_extension (self->proxy,
                                            uuid,
                                            NULL,
                                            (GAsyncReadyCallback) enable_extension_done,
                                            extension);
}

static void
disable_extension_done (ShellExtensions *proxy,
                        GAsyncResult    *res,
                        ExmExtension    *extension)
{
    GError *error = NULL;
    gboolean success;
    shell_extensions_call_disable_extension_finish (proxy, &success, res, &error);

    if (!success)
    {
        gchar *uuid;
        g_object_get (extension, "uuid", &uuid, NULL);
        if (error)
            g_critical ("Could not disable extension '%s': %s\n", uuid, error->message);
        else
            g_critical ("Could not disable extension '%s': unknown failure", uuid);
    }
}

void
exm_manager_disable_extension (ExmManager   *self,
                               ExmExtension *extension)
{
    gchar *uuid;
    g_object_get (extension, "uuid", &uuid, NULL);

    shell_extensions_call_disable_extension (self->proxy,
                                             uuid,
                                             NULL,
                                             (GAsyncReadyCallback) disable_extension_done,
                                             extension);
}

static void
remove_extension_done (ShellExtensions *proxy,
                       GAsyncResult    *res,
                       ExmExtension    *extension)
{
    GError *error = NULL;
    gboolean success;
    shell_extensions_call_uninstall_extension_finish (proxy, &success, res, &error);

    if (!success)
    {
        gchar *uuid;
        g_object_get (extension, "uuid", &uuid, NULL);
        if (error)
            g_critical ("Could not remove extension '%s': %s\n", uuid, error->message);
        else
            g_critical ("Could not remove extension '%s': unknown failure", uuid);
    }
}

void
exm_manager_remove_extension (ExmManager   *self,
                              ExmExtension *extension)
{
    gchar *uuid;
    g_object_get (extension, "uuid", &uuid, NULL);

    shell_extensions_call_uninstall_extension (self->proxy,
                                               uuid,
                                               NULL,
                                               (GAsyncReadyCallback) remove_extension_done,
                                               extension);
}

static void
open_prefs_done (ShellExtensions *proxy,
                 GAsyncResult    *res,
                 ExmExtension    *extension)
{
    GError *error = NULL;
    shell_extensions_call_launch_extension_prefs_finish (proxy, res, &error);

    if (error)
    {
        gchar *uuid;
        g_object_get (extension, "uuid", &uuid, NULL);
        g_critical ("Could not open extension preferences: %s\n", error->message);
    }
}

void
exm_manager_open_prefs (ExmManager   *self,
                        ExmExtension *extension)
{
    gchar *uuid;
    g_object_get (extension, "uuid", &uuid, NULL);

    shell_extensions_call_launch_extension_prefs (self->proxy,
                                                  uuid,
                                                  NULL,
                                                  (GAsyncReadyCallback) open_prefs_done,
                                                  extension);
}

static gboolean
list_model_contains (GListModel  *model,
                     const gchar *uuid)
{
    int n_items = g_list_model_get_n_items (model);
    for (int i = 0; i < n_items; i++)
    {
        ExmExtension *ext = g_list_model_get_item (model, i);

        gchar *cmp_uuid;
        g_object_get (ext, "uuid", &cmp_uuid, NULL);

        if (strcmp (uuid, cmp_uuid) == 0)
            return TRUE;
    }

    return FALSE;
}

gboolean
exm_manager_is_installed_uuid (ExmManager  *self,
                               const gchar *uuid)
{
    if (list_model_contains (self->user_ext_model, uuid))
        return TRUE;

    if (list_model_contains (self->system_ext_model, uuid))
        return TRUE;

    return FALSE;
}

static void
do_install_thread (GTask        *task,
                   ExmManager   *self,
                   const char   *uuid,
                   GCancellable *cancellable)
{
    GError *error = NULL;

    g_dbus_proxy_call_sync (self->proxy,
                            "InstallRemoteExtension",
                            g_variant_new ("(s)", uuid, NULL),
                            G_DBUS_CALL_FLAGS_NONE,
                            -1, cancellable, &error);

    if (error != NULL)
    {
        g_task_return_error (task, error);
        return;
    }

    g_task_return_boolean (task, TRUE);
}

void
exm_manager_install_async (ExmManager          *self,
                           const gchar         *uuid,
                           GCancellable        *cancellable,
                           GAsyncReadyCallback  callback,
                           gpointer             user_data)
{
    GTask *task;

    task = g_task_new (self, cancellable, callback, user_data);
    g_task_set_task_data (task, g_strdup (uuid), (GDestroyNotify) g_free);
    g_task_run_in_thread (task, (GTaskThreadFunc)do_install_thread);
    g_object_unref (task);
}

gboolean
exm_manager_install_finish (ExmManager    *self,
                            GAsyncResult  *result,
                            GError       **error)
{
    g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}

static void
exm_manager_class_init (ExmManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_manager_finalize;
    object_class->get_property = exm_manager_get_property;
    object_class->set_property = exm_manager_set_property;

    properties [PROP_USER_EXTENSIONS]
        = g_param_spec_object ("user-extensions",
                               "User Extensions List Model",
                               "User Extensions List Model",
                               G_TYPE_LIST_MODEL,
                               G_PARAM_READABLE);

    properties [PROP_SYSTEM_EXTENSIONS]
        = g_param_spec_object ("system-extensions",
                               "System Extensions List Model",
                               "System Extensions List Model",
                               G_TYPE_LIST_MODEL,
                               G_PARAM_READABLE);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
parse_extension_list (GVariant   *exlist,
                      GListModel **user_ext_model,
                      GListModel **system_ext_model)
{
    GListStore *user_ext_store;
    GListStore *system_ext_store;

    /* format: a{sa{sv}}
     * array of interfaces, where each interface is an array of properties
     * each interface corresponds to one extension
     * this is terrible >:(
     * see also: https://stackoverflow.com/questions/54131543/how-can-i-get-the-g-dbus-connection-signal-subscribe-function-to-tell-me-about-p
     */
    GVariantIter *iter, *iter2;
    gchar *exname, *prop_name;
    GVariant *prop_value;

    user_ext_store = g_list_store_new (EXM_TYPE_EXTENSION);
    system_ext_store = g_list_store_new (EXM_TYPE_EXTENSION);

    g_variant_get (exlist, "a{sa{sv}}", &iter);
    while (g_variant_iter_loop(iter, "{sa{sv}}", &exname, &iter2)) {
        // g_print ("Extension Discovered: %s\n", exname);

        ExmExtension *extension;

        // Well-Defined Properties
        gchar *uuid = NULL;
        gchar *display_name = NULL;
        gchar *description = NULL;
        gboolean enabled = FALSE;
        gboolean is_user = FALSE;
        gboolean has_prefs = FALSE;
        gboolean has_update = FALSE;

        while (g_variant_iter_loop(iter2, "{sv}", &prop_name, &prop_value))
        {
            // g_print (" - Property: %s=%s\n", prop_name, g_variant_print(prop_value, 0));

            // Compare with DBus property names
            if (strcmp (prop_name, "uuid") == 0)
            {
                g_variant_get (prop_value, "s", &uuid);

                // Assert that this is the same as the extension name
                g_assert (strcmp(exname, uuid) == 0);
            }
            else if (strcmp (prop_name, "name") == 0)
            {
                g_variant_get (prop_value, "s", &display_name);
            }
            else if (strcmp (prop_name, "description") == 0)
            {
                g_variant_get (prop_value, "s", &description);
            }
            else if (strcmp (prop_name, "hasPrefs") == 0)
            {
                g_variant_get (prop_value, "b", &has_prefs);
            }
            else if (strcmp (prop_name, "hasUpdate") == 0)
            {
                g_variant_get (prop_value, "b", &has_update);
            }
            else if (strcmp (prop_name, "state") == 0)
            {
                double state;
                g_variant_get (prop_value, "d", &state);
                enabled = (state == 1);
            }
            else if (strcmp (prop_name, "type") == 0)
            {
                double type;
                g_variant_get (prop_value, "d", &type);
                is_user = (type == 2);
            }
        }

        extension = exm_extension_new (uuid, display_name, description, enabled, is_user, has_prefs, has_update);
        g_list_store_append ((is_user ? user_ext_store : system_ext_store), extension);

        g_free (uuid);
        g_free (display_name);
        g_free (description);
    }
    g_variant_iter_free (iter);

    *user_ext_model = G_LIST_MODEL (user_ext_store);
    *system_ext_model = G_LIST_MODEL (system_ext_store);
}

static void
update_extension_list (ExmManager *self)
{
    GError *error = NULL;

    GVariant* exlist;
    shell_extensions_call_list_extensions_sync (self->proxy, &exlist, NULL, &error);

    if (error != NULL)
    {
        g_critical ("Could not list extensions: %s\n", error->message);
        return;
    }

    // Unref object if exists
    g_clear_object (&self->user_ext_model);
    g_clear_object (&self->system_ext_model);

    parse_extension_list (exlist, &self->user_ext_model, &self->system_ext_model);

    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_USER_EXTENSIONS]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SYSTEM_EXTENSIONS]);
}

static void
on_signal (GDBusProxy *proxy,
           gchar      *sender_name,
           gchar      *signal_name,
           GVariant   *parameters,
           ExmManager *self)
{
    g_print ("Signal received: %s\n", signal_name);
    update_extension_list (self);
}

static void
exm_manager_init (ExmManager *self)
{
    GError *error = NULL;

    self->proxy = shell_extensions_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                           G_DBUS_PROXY_FLAGS_NONE,
                                                           "org.gnome.Shell.Extensions",
                                                           "/org/gnome/Shell/Extensions",
                                                           NULL, &error);

    if (error != NULL)
    {
        g_critical ("Could not create proxy: %s\n", error->message);
        return;
    }

    update_extension_list (self);

    g_signal_connect (self->proxy, "g-signal", G_CALLBACK (on_signal), self);
}
