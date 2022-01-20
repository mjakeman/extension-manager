#include "exm-manager.h"

#include "exm-extension.h"
#include "shell-dbus-interface.h"

struct _ExmManager
{
    GObject parent_instance;

    ShellExtensions *proxy;
    GListModel *user_ext_model;
    GListModel *system_ext_model;

    const gchar *shell_version;
    gboolean extensions_enabled;
};

G_DEFINE_FINAL_TYPE (ExmManager, exm_manager, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_USER_EXTENSIONS,
    PROP_SYSTEM_EXTENSIONS,
    PROP_EXTENSIONS_ENABLED,
    PROP_SHELL_VERSION,
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
    case PROP_SHELL_VERSION:
        g_value_set_string (value, self->shell_version);
        break;
    case PROP_EXTENSIONS_ENABLED:
        g_value_set_boolean (value, self->extensions_enabled);
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
    case PROP_SHELL_VERSION:
        self->shell_version = g_value_dup_string (value);
        break;
    case PROP_EXTENSIONS_ENABLED:
        self->extensions_enabled = g_value_get_boolean (value);
        break;
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

static gpointer
list_model_get_by_uuid (GListModel  *model,
                        const gchar *uuid)
{
    int n_items = g_list_model_get_n_items (model);
    for (int i = 0; i < n_items; i++)
    {
        ExmExtension *ext = g_list_model_get_item (model, i);

        gchar *cmp_uuid;
        g_object_get (ext, "uuid", &cmp_uuid, NULL);

        if (strcmp (uuid, cmp_uuid) == 0)
            return ext;
    }

    return NULL;
}

static gboolean
list_model_contains (GListModel  *model,
                     const gchar *uuid)
{
    return list_model_get_by_uuid (model, uuid) != NULL;
}

ExmExtension *
exm_manager_get_by_uuid (ExmManager  *self,
                         const gchar *uuid)
{
    ExmExtension *result = NULL;

    if ((result = list_model_get_by_uuid (self->user_ext_model, uuid)) != NULL)
        return result;

    if ((result = list_model_get_by_uuid (self->system_ext_model, uuid)) != NULL)
        return result;

    return NULL;
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

    properties [PROP_SHELL_VERSION]
        = g_param_spec_string ("shell-version",
                               "Shell Version",
                               "Shell Version",
                               NULL,
                               G_PARAM_READWRITE);

    properties [PROP_EXTENSIONS_ENABLED]
        = g_param_spec_boolean ("extensions-enabled",
                                "Extensions Enabled",
                                "Extensions Enabled",
                                FALSE,
                                G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
parse_single_extension (ExmExtension **extension,
                        const gchar   *extension_uuid,
                        GVariantIter  *variant_iter,
                        gboolean      *is_user,
                        gboolean      *is_uninstall_operation)
{
    gchar *prop_name;
    GVariant *prop_value;

    // Well-Defined Properties
    gchar *uuid = NULL;
    gchar *display_name = NULL;
    gchar *description = NULL;
    gboolean enabled = FALSE;
    gboolean has_prefs = FALSE;
    gboolean has_update = FALSE;
    gboolean can_change = TRUE;

    *is_user = FALSE;
    *is_uninstall_operation = FALSE;

    if (extension && *extension)
    {
        const gchar *uuid_cmp;
        g_object_get (*extension, "uuid", &uuid_cmp, NULL);
        g_assert (strcmp (extension_uuid, uuid_cmp) == 0);
    }
    else
    {
        *extension = exm_extension_new (extension_uuid);
    }

    g_debug ("Found extension '%s' with properties:\n", uuid);

    while (g_variant_iter_loop (variant_iter, "{sv}", &prop_name, &prop_value))
    {
        g_debug (" - Property: %s=%s\n", prop_name, g_variant_print(prop_value, 0));

        // Compare with DBus property names
        if (strcmp (prop_name, "uuid") == 0)
        {
            g_variant_get (prop_value, "s", &uuid);

            // Assert that this is the same as the extension uuid
            g_assert (strcmp(extension_uuid, uuid) == 0);
        }
        else if (strcmp (prop_name, "type") == 0)
        {
            double type;
            g_variant_get (prop_value, "d", &type);
            *is_user = (type == 2);
        }
        else if (strcmp (prop_name, "state") == 0)
        {
            double state;
            g_variant_get (prop_value, "d", &state);
            enabled = (state == 1);

            if (state == 99)
            {
                *is_uninstall_operation = TRUE;
            }
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
        else if (strcmp (prop_name, "canChange") == 0)
        {
            g_variant_get (prop_value, "b", &can_change);
        }
    }

    g_object_set (*extension,
                  "display-name", display_name,
                  "description", description,
                  "enabled", enabled,
                  "is-user", *is_user,
                  "has-prefs", has_prefs,
                  "has-update", has_update,
                  "can-change", can_change,
                  NULL);

    g_free (uuid);
    g_free (display_name);
    g_free (description);
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
    gchar *exname;

    user_ext_store = g_list_store_new (EXM_TYPE_EXTENSION);
    system_ext_store = g_list_store_new (EXM_TYPE_EXTENSION);

    g_variant_get (exlist, "a{sa{sv}}", &iter);
    while (g_variant_iter_loop (iter, "{sa{sv}}", &exname, &iter2)) {
        gboolean is_user;
        gboolean is_uninstall_operation;
        ExmExtension *extension = NULL;

        parse_single_extension (&extension, exname, iter2, &is_user, &is_uninstall_operation);

        if (is_uninstall_operation)
        {
            g_error ("Invalid extension: '%s' is being uninstalled.", exname);
            continue;
        }

        g_list_store_append ((is_user ? user_ext_store : system_ext_store), extension);
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

static gboolean
is_extension_equal (ExmExtension *a, ExmExtension *b)
{
    const gchar *uuid_a, *uuid_b;
    g_object_get (a, "uuid", &uuid_a, NULL);
    g_object_get (b, "uuid", &uuid_b, NULL);

    return strcmp (uuid_a, uuid_b) == 0;
}

static gboolean
replace_extension_in_list_store (GListStore   *store,
                                 ExmExtension *to_replace)
{
    guint position;
    ExmExtension *to_delete;

    if (g_list_store_find_with_equal_func (store, to_replace, (GEqualFunc)is_extension_equal, &position))
    {
        to_delete = g_list_model_get_item (G_LIST_MODEL (store), position);
        g_list_store_remove (store, position);
        g_list_store_insert (store, position, to_replace);
        g_object_unref (to_delete);

        return TRUE;
    }

    return FALSE;
}

static void
on_state_changed (ShellExtensions *object,
                  const gchar     *arg_uuid,
                  GVariant        *arg_state,
                  ExmManager      *self)
{
    ExmExtension *extension;
    gboolean is_user;
    gboolean is_new;
    gboolean is_uninstall_operation;
    GListStore *list_store;

    g_debug ("State Changed for extension '%s'\n", arg_uuid);

    // Parse the new extension state and update only that element in
    // the list model. This will automatically update any bound
    // listboxes or listviews.

    // This is NULL if it does not exist
    extension = exm_manager_get_by_uuid (self, arg_uuid);
    is_new = (extension == NULL);

    GVariantIter *iter;
    g_variant_get (arg_state, "a{sv}", &iter);
    parse_single_extension (&extension, arg_uuid, iter, &is_user, &is_uninstall_operation);
    g_variant_iter_free (iter);

    list_store = G_LIST_STORE (is_user ? self->user_ext_model : self->system_ext_model);

    if (is_new)
    {
        g_list_store_append (list_store, extension);
        return;
    }

    if (is_uninstall_operation)
    {
        guint position;
        if (g_list_store_find_with_equal_func (list_store, extension, (GEqualFunc)is_extension_equal, &position))
            g_list_store_remove (list_store, position);

        return;
    }
}

static void
on_status_changed (ShellExtensions *object,
                   const gchar     *arg_uuid,
                   gint             arg_state,
                   const gchar     *arg_error,
                   ExmManager      *self)
{
    g_debug ("Status Changed (Unhandled) for extension '%s'\n", arg_uuid);

    // TODO: What's this for?
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

    g_object_bind_property (self->proxy, "shell-version",
                            self, "shell-version",
                            G_BINDING_SYNC_CREATE);

    g_object_bind_property (self->proxy, "user-extensions-enabled",
                            self, "extensions-enabled",
                            G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);

    update_extension_list (self);

    g_signal_connect (self->proxy, "extension-state-changed", G_CALLBACK (on_state_changed), self);
    g_signal_connect (self->proxy, "extension-status-changed", G_CALLBACK (on_status_changed), self);
}
