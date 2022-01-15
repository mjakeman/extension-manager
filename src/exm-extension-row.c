#include "exm-extension-row.h"

struct _ExmExtensionRow
{
    AdwExpanderRow parent_instance;

    ExmExtension *extension;

    GtkButton *remove_btn;
    GtkButton *prefs_btn;
    GtkButton *description_label;
    GtkSwitch *ext_toggle;
};

G_DEFINE_FINAL_TYPE (ExmExtensionRow, exm_extension_row, ADW_TYPE_EXPANDER_ROW)

enum {
    PROP_0,
    PROP_EXTENSION,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmExtensionRow *
exm_extension_row_new (ExmExtension *extension)
{
    return g_object_new (EXM_TYPE_EXTENSION_ROW,
                         "extension", extension,
                         NULL);
}

static void
exm_extension_row_finalize (GObject *object)
{
    ExmExtensionRow *self = (ExmExtensionRow *)object;

    G_OBJECT_CLASS (exm_extension_row_parent_class)->finalize (object);
}

static void
exm_extension_row_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
    ExmExtensionRow *self = EXM_EXTENSION_ROW (object);

    switch (prop_id)
    {
    case PROP_EXTENSION:
        g_value_set_object (value, self->extension);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_extension_row_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
    ExmExtensionRow *self = EXM_EXTENSION_ROW (object);

    switch (prop_id)
    {
    case PROP_EXTENSION:
        self->extension = g_value_get_object (value);
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

static void
exm_extension_row_constructed (GObject *object)
{
    // TODO: This big block of property assignments is currently copy/pasted
    // from ExmExtension. We can replace this with GtkExpression lookups
    // once blueprint-compiler supports expressions.
    // (See https://gitlab.gnome.org/jwestman/blueprint-compiler/-/issues/5)

    ExmExtensionRow *self = EXM_EXTENSION_ROW (object);

    gchar *name, *uuid, *description;
    gboolean enabled, has_prefs, is_user;
    g_object_get (self->extension,
                  "display-name", &name,
                  "uuid", &uuid,
                  "description", &description,
                  "enabled", &enabled,
                  "has-prefs", &has_prefs,
                  "is-user", &is_user,
                  NULL);

    g_object_set (self, "title", name, "subtitle", uuid, NULL);
    g_object_set (self->description_label, "label", description, NULL);
    g_object_set (self->prefs_btn, "visible", has_prefs, NULL);
    g_object_set (self->remove_btn, "visible", is_user, NULL);

    // TODO: Use a property binding for this
    g_object_set (self->ext_toggle, "state", enabled, NULL);

    // TODO: These signal connections can be removed once blueprint-compiler
    // supports signal object parameters.
    // (See https://gitlab.gnome.org/jwestman/blueprint-compiler/-/merge_requests/4)
    g_signal_connect (self->remove_btn, "clicked", G_CALLBACK (on_remove), uuid);
    g_signal_connect (self->prefs_btn, "clicked", G_CALLBACK (on_open_prefs), uuid);
    g_signal_connect (self->ext_toggle, "state-set", G_CALLBACK (on_state_toggled), uuid);
}

static void
exm_extension_row_class_init (ExmExtensionRowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_extension_row_finalize;
    object_class->get_property = exm_extension_row_get_property;
    object_class->set_property = exm_extension_row_set_property;
    object_class->constructed = exm_extension_row_constructed;

    properties [PROP_EXTENSION] =
        g_param_spec_object ("extension",
                             "Extension",
                             "Extension",
                             EXM_TYPE_EXTENSION,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-extension-row.ui");

    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, description_label);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, prefs_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, remove_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, ext_toggle);

    gtk_widget_class_bind_template_callback (widget_class, on_open_prefs);
    gtk_widget_class_bind_template_callback (widget_class, on_remove);
    gtk_widget_class_bind_template_callback (widget_class, on_state_toggled);
}

static void
exm_extension_row_init (ExmExtensionRow *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}
