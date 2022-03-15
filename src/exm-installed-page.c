#include "exm-installed-page.h"

#include "exm-extension-row.h"

#include "local/exm-manager.h"

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
};

G_DEFINE_FINAL_TYPE (ExmInstalledPage, exm_installed_page, GTK_TYPE_WIDGET)

enum {
    PROP_0,
    PROP_MANAGER,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmInstalledPage *
exm_installed_page_new (void)
{
    return g_object_new (EXM_TYPE_INSTALLED_PAGE, NULL);
}

static void
exm_installed_page_finalize (GObject *object)
{
    ExmInstalledPage *self = (ExmInstalledPage *)object;

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
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
on_state_toggled (GtkSwitch *toggle,
                  gboolean   state,
                  gchar     *uuid)
{
    gtk_widget_activate_action (GTK_WIDGET (toggle),
                                "ext.state-set",
                                "(sb)", uuid, state);

    return FALSE;
}

static void
on_open_prefs (GtkButton *button,
               gchar     *uuid)
{
    gtk_widget_activate_action (GTK_WIDGET (button),
                                "ext.open-prefs",
                                "s", uuid);
}

static void
on_remove (GtkButton *button,
           gchar     *uuid)
{
    gtk_widget_activate_action (GTK_WIDGET (button),
                                "ext.remove",
                                "s", uuid);
}

static GtkWidget *
widget_factory (ExmExtension* extension)
{
    ExmExtensionRow *row;

    row = exm_extension_row_new (extension);

    return GTK_WIDGET (row);
}

static void
bind_list_box (GtkListBox *list_box,
               GListModel *model)
{
    GtkExpression *expression;
    GtkStringSorter *sorter;
    GtkSortListModel *sorted_model;

    // Sort alphabetically
    expression = gtk_property_expression_new (EXM_TYPE_EXTENSION, NULL, "display-name");
    sorter = gtk_string_sorter_new (expression);
    sorted_model = gtk_sort_list_model_new (model, GTK_SORTER (sorter));

    gtk_list_box_bind_model (list_box, G_LIST_MODEL (sorted_model),
                             (GtkListBoxCreateWidgetFunc) widget_factory,
                             NULL, NULL);
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
    label = g_strdup_printf(ngettext("One extension will be updated on next login.",
                                     "%d extensions will be updated on next login.",
                                     n_updates), n_updates);

    gtk_label_set_label (self->num_updates_label, label);
    g_free (label);

    // Short delay to draw user attention
    g_timeout_add (500, G_SOURCE_FUNC (show_updates_bar), self);
}

static void
on_bind_manager (ExmInstalledPage *self)
{
    GListModel *user_ext_model;
    GListModel *system_ext_model;

    g_object_get (self->manager,
                  "user-extensions", &user_ext_model,
                  "system-extensions", &system_ext_model,
                  NULL);

    bind_list_box (self->user_list_box, user_ext_model);
    bind_list_box (self->system_list_box, system_ext_model);

    g_object_bind_property (self->manager,
                            "extensions-enabled",
                            self->user_list_box,
                            "sensitive",
                            G_BINDING_SYNC_CREATE);

    g_object_bind_property (self->manager,
                            "extensions-enabled",
                            self->system_list_box,
                            "sensitive",
                            G_BINDING_SYNC_CREATE);

    g_signal_connect (self->manager,
                      "updates-available",
                      G_CALLBACK (on_updates_available),
                      self);

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

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-installed-page.ui");
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, user_list_box);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, system_list_box);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, num_updates_label);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, updates_action_bar);

    gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
exm_installed_page_init (ExmInstalledPage *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    g_signal_connect (self,
                      "notify::manager",
                      G_CALLBACK (on_bind_manager),
                      NULL);
}
