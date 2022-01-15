#include "exm-installed-page.h"

#include "local/exm-manager.h"

#include <glib/gi18n.h>

struct _ExmInstalledPage
{
    GtkWidget parent_instance;

    ExmManager *manager;

    // Template Widgets
    GtkListBox          *user_list_box;
    GtkListBox          *system_list_box;
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
    GtkWidget *row;
    GtkWidget *label;
    GtkWidget *toggle;
    GtkWidget *prefs;
    GtkWidget *remove;

    gchar *name, *uuid, *description;
    gboolean enabled, has_prefs, is_user;
    g_object_get (extension,
                  "display-name", &name,
                  "uuid", &uuid,
                  "description", &description,
                  "enabled", &enabled,
                  "has-prefs", &has_prefs,
                  "is-user", &is_user,
                  NULL);

    name = g_markup_escape_text (name, -1);

    row = adw_expander_row_new ();

    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), name);
    adw_expander_row_set_subtitle (ADW_EXPANDER_ROW (row), uuid);

    toggle = gtk_switch_new ();
    gtk_switch_set_state (GTK_SWITCH (toggle), enabled);
    gtk_widget_set_valign (toggle, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (toggle, GTK_ALIGN_CENTER);
    adw_expander_row_add_action (ADW_EXPANDER_ROW (row), toggle);
    g_signal_connect (toggle, "state-set", G_CALLBACK (on_state_toggled), uuid);

    if (has_prefs)
    {
        prefs = gtk_button_new_from_icon_name ("settings-symbolic");
        gtk_widget_set_valign (prefs, GTK_ALIGN_CENTER);
        gtk_widget_set_halign (prefs, GTK_ALIGN_CENTER);
        g_signal_connect (prefs, "clicked", G_CALLBACK (on_open_prefs), uuid);
        adw_expander_row_add_action (ADW_EXPANDER_ROW (row), prefs);
    }

    label = gtk_label_new (description);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_label_set_wrap (GTK_LABEL (label), GTK_WRAP_WORD);
    gtk_widget_add_css_class (label, "content");
    adw_expander_row_add_row (ADW_EXPANDER_ROW (row), label);

    if (is_user)
    {
        remove = gtk_button_new_with_label (_("Remove"));
        gtk_widget_add_css_class (remove, "destructive-action");
        gtk_widget_set_valign (remove, GTK_ALIGN_CENTER);
        gtk_widget_set_halign (remove, GTK_ALIGN_END);
        g_signal_connect (remove, "clicked", G_CALLBACK (on_remove), uuid);
        adw_expander_row_add_row (ADW_EXPANDER_ROW (row), remove);
    }

    return row;
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
