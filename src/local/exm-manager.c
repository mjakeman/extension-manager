#include "exm-manager.h"

#include "exm-extension.h"
#include "shell-dbus-interface.h"

#include "../exm-types.h"
#include "../exm-enums.h"

struct _ExmManager
{
    GObject parent_instance;

    ShellExtensions *proxy;
    GListModel *user_ext_model;
    GListModel *system_ext_model;

    const gchar *shell_version;
    gboolean extensions_enabled;

    guint update_callback_id;
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

enum {
    SIGNAL_0,
    SIGNAL_UPDATES_AVAILABLE,
    SIGNAL_ERROR_OCCURRED,
    N_SIGNALS
};

static guint signals [N_SIGNALS];

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

typedef struct
{
    ExmExtension *extension;
    ExmManager *manager;
} ExmCallbackData;

static ExmCallbackData *
create_callback_data (ExmManager *manager,
                      ExmExtension *extension)
{
    ExmCallbackData *data;
    data = g_slice_new0 (ExmCallbackData);

    data->manager = g_object_ref (manager);
    data->extension = g_object_ref (extension);

	return data;
}

static void
free_callback_data (ExmCallbackData *data)
{
	if (!data)
		return;

    g_clear_object (&data->manager);
    g_clear_object (&data->extension);

    g_slice_free (ExmCallbackData, data);
}

#define notify_error(self_, f_, ...); \
    {\
        char *error_text;\
        error_text = g_strdup_printf (f_, __VA_ARGS__);\
        g_critical ("%s", error_text);\
        g_signal_emit (G_OBJECT (self_), signals [SIGNAL_ERROR_OCCURRED], 0, error_text);\
        g_free (error_text);\
    }\

static void
enable_extension_done (ShellExtensions *proxy,
                       GAsyncResult    *res,
                       ExmCallbackData *data)
{
    GError *error = NULL;
    gboolean success;
    shell_extensions_call_enable_extension_finish (proxy, &success, res, &error);

    if (!success)
    {
        gchar *uuid;
        g_object_get (data->extension, "uuid", &uuid, NULL);
        if (error)
        {
            notify_error (data->manager, "Could not enable extension '%s': %s\n", uuid, error->message);
        }
        else
        {
            notify_error (data->manager, "Could not enable extension '%s': unknown failure", uuid);
        }
    }

    free_callback_data (data);
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
                                            create_callback_data (self, extension));
}

static void
disable_extension_done (ShellExtensions *proxy,
                        GAsyncResult    *res,
                        ExmCallbackData *data)
{
    GError *error = NULL;
    gboolean success;
    shell_extensions_call_disable_extension_finish (proxy, &success, res, &error);

    if (!success)
    {
        gchar *uuid;
        g_object_get (data->extension, "uuid", &uuid, NULL);
        if (error)
        {
            notify_error (data->manager, "Could not disable extension '%s': %s\n", uuid, error->message);
        }
        else
        {
            notify_error (data->manager, "Could not disable extension '%s': unknown failure", uuid);
        }
    }

    free_callback_data (data);
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
                                             create_callback_data (self, extension));
}

static void
remove_extension_done (ShellExtensions *proxy,
                       GAsyncResult    *res,
                       ExmCallbackData *data)
{
    GError *error = NULL;
    gboolean success;
    shell_extensions_call_uninstall_extension_finish (proxy, &success, res, &error);

    if (!success)
    {
        gchar *uuid;
        g_object_get (data->extension, "uuid", &uuid, NULL);
        if (error)
        {
            notify_error (data->manager, "Could not remove extension '%s': %s\n", uuid, error->message);
        }
        else
        {
            notify_error (data->manager, "Could not remove extension '%s': unknown failure", uuid);
        }
    }

    free_callback_data (data);
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
                                               create_callback_data (self, extension));
}

static void
open_prefs_done (ShellExtensions *proxy,
                 GAsyncResult    *res,
                 ExmCallbackData *data)
{
    GError *error = NULL;
    shell_extensions_call_launch_extension_prefs_finish (proxy, res, &error);

    // TODO: Don't enable until we can export the window handle over dbus
    /*if (error)
    {
        gchar *uuid;
        g_object_get (data->extension, "uuid", &uuid, NULL);
        notify_error (data->manager, "Could not open extension '%s' preferences: %s\n", uuid, error->message);
    }*/

    free_callback_data (data);
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
                                                  create_callback_data (self, extension));
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

    g_dbus_proxy_call_sync (G_DBUS_PROXY (self->proxy),
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

static int
list_model_get_number_of_updates (GListModel *model)
{
    int num_updates = 0;

    int n_items = g_list_model_get_n_items (model);
    for (int i = 0; i < n_items; i++)
    {
        ExmExtension *ext = g_list_model_get_item (model, i);

        gboolean has_update;
        g_object_get (ext, "has-update", &has_update, NULL);

        if (has_update)
            num_updates++;
    }

    return num_updates;
}

static guint
notify_extension_updates (ExmManager *self)
{
    // Checks for updates in the background and if found then
    // emits the 'updates-available' signal with the number of updates
    int n_updates = 0;

    n_updates += list_model_get_number_of_updates (self->user_ext_model);
    n_updates += list_model_get_number_of_updates (self->system_ext_model);

    g_info ("There are %d new updates available.", n_updates);

    if (n_updates > 0)
        g_signal_emit (G_OBJECT (self), signals [SIGNAL_UPDATES_AVAILABLE], 0, n_updates);

    self->update_callback_id = 0;

    return G_SOURCE_REMOVE;
}

static void
queue_notify_extension_updates (ExmManager *self)
{
    // See: notify_extension_updates

    if (self->update_callback_id != 0)
        return;

    self->update_callback_id = g_timeout_add (0, G_SOURCE_FUNC (notify_extension_updates), self);
}

static void
check_for_updates_done (ShellExtensions *proxy,
                        GAsyncResult    *res,
                        ExmManager      *manager)
{
    GError *error = NULL;
    shell_extensions_call_check_for_updates_finish (proxy, res, &error);

    if (error)
    {
        notify_error (manager, "Could not check for updates: %s\n", error->message);
    }

    // Notify the user if updates are detected
    queue_notify_extension_updates (manager);
}

void
exm_manager_check_for_updates (ExmManager *self)
{
    shell_extensions_call_check_for_updates (self->proxy,
                                             NULL,
                                             (GAsyncReadyCallback) check_for_updates_done,
                                             self);
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

    signals [SIGNAL_UPDATES_AVAILABLE]
        = g_signal_new ("updates-available",
                        G_TYPE_FROM_CLASS (object_class),
                        G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE|G_SIGNAL_NO_HOOKS,
                        0, NULL, NULL, NULL,
                        G_TYPE_NONE, 1,
                        G_TYPE_INT);

    signals [SIGNAL_ERROR_OCCURRED]
        = g_signal_new ("error-occurred",
                        G_TYPE_FROM_CLASS (object_class),
                        G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE|G_SIGNAL_NO_HOOKS,
                        0, NULL, NULL, NULL,
                        G_TYPE_NONE, 1,
                        G_TYPE_STRING);
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
    ExmExtensionState state;
    ExmExtensionType type;
    gchar *version = NULL;
    gchar *error_msg = NULL;

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
            gdouble val;
            g_variant_get (prop_value, "d", &val);
            type = (ExmExtensionType)val;
        }
        else if (strcmp (prop_name, "state") == 0)
        {
            gdouble val;
            g_variant_get (prop_value, "d", &val);
            state = (ExmExtensionState)val;
        }
        else if (strcmp (prop_name, "enabled") == 0)
        {
            g_variant_get (prop_value, "b", &enabled);
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
        else if (strcmp (prop_name, "version") == 0)
        {
            gdouble val;
            g_variant_get (prop_value, "d", &val);
            version = g_strdup_printf ("%d", (gint)val);
        }
        else if (strcmp (prop_name, "error") == 0)
        {
            g_variant_get (prop_value, "s", &error_msg);
        }
    }

    *is_user = (type == EXM_EXTENSION_TYPE_PER_USER);
    *is_uninstall_operation = (state == EXM_EXTENSION_STATE_UNINSTALLED);

    g_object_set (*extension,
                  "display-name", display_name,
                  "description", description,
                  "state", state,
                  "enabled", enabled,
                  "is-user", *is_user,
                  "has-prefs", has_prefs,
                  "has-update", has_update,
                  "can-change", can_change,
                  "version", version,
                  "error-msg", error_msg,
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
        notify_error (self, "Could not list extensions: %s\n", error->message);
        return;
    }

    // Unref object if exists
    g_clear_object (&self->user_ext_model);
    g_clear_object (&self->system_ext_model);

    parse_extension_list (exlist, &self->user_ext_model, &self->system_ext_model);

    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_USER_EXTENSIONS]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SYSTEM_EXTENSIONS]);

    queue_notify_extension_updates (self);
}

static gboolean
is_extension_equal (ExmExtension *a, ExmExtension *b)
{
    const gchar *uuid_a, *uuid_b;
    g_object_get (a, "uuid", &uuid_a, NULL);
    g_object_get (b, "uuid", &uuid_b, NULL);

    return strcmp (uuid_a, uuid_b) == 0;
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

    // Emit items-changed signal to re-sort extension list
    {
        guint position;
        if (g_list_store_find_with_equal_func (list_store, extension, (GEqualFunc)is_extension_equal, &position))
            g_list_model_items_changed (G_LIST_MODEL (list_store), position, 1, 1);
    }

    // If the extension that has changed has an update, then
    // one or more extensions have updates available. Lazily
    // check the exact number and emit the 'updates-available'
    // signal.
    gboolean has_update;
    g_object_get (extension, "has-update", &has_update, NULL);

    if (has_update)
        queue_notify_extension_updates (self);
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
        char *error_text;
        error_text = g_strdup_printf ("Could not create proxy: %s\n", error->message);
        notify_error (self, "%s", error_text);
        return;
    }

    self->update_callback_id = 0;

    g_object_bind_property (self->proxy, "shell-version",
                            self, "shell-version",
                            G_BINDING_SYNC_CREATE);

    g_object_bind_property (self->proxy, "user-extensions-enabled",
                            self, "extensions-enabled",
                            G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);

    update_extension_list (self);

    g_signal_connect (self->proxy,
                      "extension-state-changed",
                      G_CALLBACK (on_state_changed),
                      self);
}
