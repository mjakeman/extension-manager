#include "exm-installed-page.h"

#include "exm-extension-row.h"

#include "local/exm-manager.h"
#include "exm-enums.h"
#include "exm-types.h"

#include "exm-config.h"

#include <glib/gi18n.h>

struct _ExmInstalledPage
{
    GtkWidget parent_instance;

    ExmManager *manager;

    // Template Widgets
    GtkListBox *user_list_box;
    GtkListBox *system_list_box;
    GtkLabel *num_updates_label;
    GtkRevealer *updates_action_bar;
    AdwSwitchRow *global_toggle;

    gboolean sort_enabled_first;
};

G_DEFINE_FINAL_TYPE (ExmInstalledPage, exm_installed_page, GTK_TYPE_WIDGET)

enum {
    PROP_0,
    PROP_MANAGER,
    PROP_SORT_ENABLED_FIRST,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
invalidate_model_bindings (ExmInstalledPage *self);

ExmInstalledPage *
exm_installed_page_new (void)
{
    return g_object_new (EXM_TYPE_INSTALLED_PAGE, NULL);
}

static void
exm_installed_page_finalize (GObject *object)
{
    GtkWidget *child;
    ExmInstalledPage *self = (ExmInstalledPage *)object;

    child = gtk_widget_get_first_child (GTK_WIDGET (self));
    gtk_widget_unparent (child);

    G_OBJECT_CLASS (exm_installed_page_parent_class)->finalize (object);
}

static void
exm_installed_page_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
    ExmInstalledPage *self = EXM_INSTALLED_PAGE (object);

    switch (prop_id)
    {
    case PROP_MANAGER:
        g_value_set_object (value, self->manager);
        break;
    case PROP_SORT_ENABLED_FIRST:
        g_value_set_boolean (value, self->sort_enabled_first);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_installed_page_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
    ExmInstalledPage *self = EXM_INSTALLED_PAGE (object);

    switch (prop_id)
    {
    case PROP_MANAGER:
        self->manager = g_value_get_object (value);
        break;
    case PROP_SORT_ENABLED_FIRST:
        self->sort_enabled_first = g_value_get_boolean (value);
        invalidate_model_bindings (self);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static GtkWidget *
widget_factory (ExmExtension     *extension,
                ExmInstalledPage *self)
{
    ExmExtensionRow *row;
    g_return_val_if_fail (EXM_IS_EXTENSION (extension), GTK_WIDGET (NULL));
    g_return_val_if_fail (EXM_IS_INSTALLED_PAGE (self), GTK_WIDGET (NULL));

    row = exm_extension_row_new (extension, self->manager);
    return GTK_WIDGET (row);
}

static int
compare_enabled (ExmExtension *this, ExmExtension *other)
{
    g_return_val_if_fail (EXM_IS_EXTENSION (this), 2);
    g_return_val_if_fail (EXM_IS_EXTENSION (other), 2); // Crash

    ExmExtensionState this_state;
    ExmExtensionState other_state;

    g_object_get (this, "state", &this_state, NULL);
    g_object_get (other, "state", &other_state, NULL);

    gboolean this_enabled = (this_state == EXM_EXTENSION_STATE_ACTIVE);
    gboolean other_enabled = (other_state == EXM_EXTENSION_STATE_ACTIVE);

    if ((this_enabled && other_enabled) || (!this_enabled && !other_enabled))
        return 0;
    else if (this_enabled && !other_enabled)
        return -1;
    else if (!this_enabled && other_enabled)
        return 1;
}

static void
bind_list_box (GtkListBox       *list_box,
               GListModel       *model,
               gboolean          sort_enabled_first,
               ExmInstalledPage *self)
{
    GtkExpression *expression;
    GtkStringSorter *alphabetical_sorter;
    GtkSortListModel *sorted_model;

    g_return_if_fail (GTK_IS_LIST_BOX (list_box));
    g_return_if_fail (G_IS_LIST_MODEL (model));

    // Sort alphabetically
    expression = gtk_property_expression_new (EXM_TYPE_EXTENSION, NULL, "display-name");
    alphabetical_sorter = gtk_string_sorter_new (expression);

    if (sort_enabled_first)
    {
        GtkCustomSorter *enabled_sorter;
        GtkMultiSorter *multi_sorter;

        // Sort by enabled
        enabled_sorter = gtk_custom_sorter_new ((GCompareDataFunc) compare_enabled, NULL, NULL);

        multi_sorter = gtk_multi_sorter_new ();
        gtk_multi_sorter_append (multi_sorter, GTK_SORTER (enabled_sorter));
        gtk_multi_sorter_append (multi_sorter, GTK_SORTER (alphabetical_sorter));

        sorted_model = gtk_sort_list_model_new (model, GTK_SORTER (multi_sorter));
    }
    else
    {
        sorted_model = gtk_sort_list_model_new (model, GTK_SORTER (alphabetical_sorter));
    }

    gtk_list_box_bind_model (list_box, G_LIST_MODEL (sorted_model),
                             (GtkListBoxCreateWidgetFunc) widget_factory,
                             self, NULL);
}

static guint
show_updates_bar (ExmInstalledPage *self)
{
    gtk_revealer_set_reveal_child (self->updates_action_bar, TRUE);

    return G_SOURCE_REMOVE;
}

static void
on_updates_available (ExmManager       *manager,
                      int               n_updates,
                      ExmInstalledPage *self)
{
    char *label;

    // Translators: '%d' = number of extensions that will be updated
    label = g_strdup_printf(ngettext("One extension will be updated on next login",
                                     "%d extensions will be updated on next login",
                                     n_updates), n_updates);

    gtk_label_set_label (self->num_updates_label, label);
    g_free (label);

    // Short delay to draw user attention
    g_timeout_add (500, G_SOURCE_FUNC (show_updates_bar), self);
}

static void
invalidate_model_bindings (ExmInstalledPage *self)
{
    GListModel *user_ext_model;
    GListModel *system_ext_model;

    if (!self->manager)
        return;

    g_object_get (self->manager,
                  "user-extensions", &user_ext_model,
                  "system-extensions", &system_ext_model,
                  NULL);

    if (user_ext_model)
        bind_list_box (self->user_list_box,
                       user_ext_model,
                       self->sort_enabled_first,
                       self);

    if (system_ext_model)
        bind_list_box (self->system_list_box,
                       system_ext_model,
                       self->sort_enabled_first,
                       self);
}

static void
on_bind_manager (ExmInstalledPage *self)
{
    // Bind (or rebind) models
    invalidate_model_bindings (self);

    g_signal_connect (self->manager,
                      "updates-available",
                      G_CALLBACK (on_updates_available),
                      self);

    g_object_bind_property (self->manager,
                            "extensions-enabled",
                            self->global_toggle,
                            "active",
                            G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);

    // Check if updates are available
    // NOTE: We need to do this *after* connecting the signal
    // handler above, otherwise we will not be notified.
    exm_manager_check_for_updates (self->manager);
}

static void
exm_installed_page_class_init (ExmInstalledPageClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_installed_page_finalize;
    object_class->get_property = exm_installed_page_get_property;
    object_class->set_property = exm_installed_page_set_property;

    properties [PROP_MANAGER]
        = g_param_spec_object ("manager",
                               "Manager",
                               "Manager",
                               EXM_TYPE_MANAGER,
                               G_PARAM_READWRITE);

    properties [PROP_SORT_ENABLED_FIRST]
        = g_param_spec_boolean ("sort-enabled-first",
                                "Sort Enabled First",
                                "Sort Enabled First",
                                FALSE,
                                G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-installed-page.ui");
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, user_list_box);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, system_list_box);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, num_updates_label);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, updates_action_bar);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, global_toggle);

    gtk_widget_class_bind_template_callback (widget_class, on_bind_manager);

    gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static GtkWidget *
create_user_placeholder ()
{
    GtkWidget *row, *button, *icon;

    row = adw_action_row_new ();
    button = gtk_button_new_with_label (_("Browse"));
    icon = gtk_image_new_from_icon_name ("globe-symbolic");

    gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (button, GTK_ALIGN_CENTER);

    gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.show-page");
    gtk_actionable_set_action_target (GTK_ACTIONABLE (button), "s", "browse");

    adw_action_row_add_prefix (ADW_ACTION_ROW (row), icon);
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row),
                                   _("No User Extensions Installed"));
    adw_action_row_add_suffix (ADW_ACTION_ROW (row), button);

    return row;
}

static GtkWidget *
create_system_placeholder ()
{
    GtkWidget *row, *icon;

    row = adw_action_row_new ();
    icon = gtk_image_new_from_icon_name ("settings-symbolic");
    adw_action_row_add_prefix (ADW_ACTION_ROW (row), icon);
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row),
                                   _("No System Extensions Installed"));

    return row;

}

static void
exm_installed_page_init (ExmInstalledPage *self)
{
    GSettings *settings;

    gtk_widget_init_template (GTK_WIDGET (self));

    settings = g_settings_new (APP_ID);

    g_settings_bind (settings, "sort-enabled-first",
                     self, "sort-enabled-first",
                     G_SETTINGS_BIND_GET);

    g_object_unref (settings);

    gtk_list_box_set_placeholder (self->user_list_box,
                                  create_user_placeholder ());

    gtk_list_box_set_placeholder (self->system_list_box,
                                  create_system_placeholder ());


}
