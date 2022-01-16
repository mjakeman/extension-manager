#include "exm-extension-row.h"

struct _ExmExtensionRow
{
    AdwExpanderRow parent_instance;

    GSimpleActionGroup *action_group;

    ExmExtension *extension;
    gchar *uuid;

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
        if (self->extension)
        {
            g_object_get (self->extension,
                          "uuid", &self->uuid,
                          NULL);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

void
update_enable_state (GObject         *self,
                     GParamSpec      *pspec,
                     ExmExtensionRow *row)
{
    // We update the state of the action without activating it. If we activate
    // it, then it will go back to gnome-shell and explicitly enable/disable
    // the extension. We do not want this behaviour as it messes with the global
    // extension toggle.

    gboolean new_state;
    g_object_get (self, "enabled", &new_state, NULL);

    GAction *action = g_action_map_lookup_action (G_ACTION_MAP (row->action_group), "state-set");
    g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (new_state));
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

    // One way binding from extension ("source of truth") to switch
    g_signal_connect (self->extension, "notify::enabled", G_CALLBACK (update_enable_state), self);

    GAction *action;

    action = g_action_map_lookup_action (G_ACTION_MAP (self->action_group), "state-set");
    g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (enabled));

    action = g_action_map_lookup_action (G_ACTION_MAP (self->action_group), "open-prefs");
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_prefs);

    action = g_action_map_lookup_action (G_ACTION_MAP (self->action_group), "remove");
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action), is_user);
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
}

static void
state_changed (GSimpleAction   *action,
               GVariant        *new_value,
               ExmExtensionRow *self)
{
    GVariant *variant;
    gboolean enabled;

    g_return_if_fail (self->extension);

    variant = g_action_get_state (G_ACTION (action));
    enabled = g_variant_get_boolean (variant);

    gtk_widget_activate_action (GTK_WIDGET (self),
                                "ext.state-set",
                                "(sb)", self->uuid, !enabled);

    g_simple_action_set_state (action, new_value);
}

static void
open_prefs (GSimpleAction   *action,
            GVariant        *new_value,
            ExmExtensionRow *self)
{
    g_return_if_fail (self->extension);

    gtk_widget_activate_action (GTK_WIDGET (self),
                                "ext.open-prefs",
                                "s", self->uuid);
}

static void
uninstall (GSimpleAction   *action,
           GVariant        *new_value,
           ExmExtensionRow *self)
{
    g_return_if_fail (self->extension);

    gtk_widget_activate_action (GTK_WIDGET (self),
                                "ext.remove",
                                "s", self->uuid);
}

static void
exm_extension_row_init (ExmExtensionRow *self)
{
    GSimpleAction *state_action;
    GSimpleAction *open_prefs_action;
    GSimpleAction *remove_action;

    gtk_widget_init_template (GTK_WIDGET (self));

    // Define Actions
    self->action_group = g_simple_action_group_new ();

    state_action = g_simple_action_new_stateful ("state-set", NULL, g_variant_new_boolean (TRUE));
    g_signal_connect (state_action, "change-state", G_CALLBACK (state_changed), self);

    open_prefs_action = g_simple_action_new ("open-prefs", NULL);
    g_signal_connect (open_prefs_action, "activate", G_CALLBACK (open_prefs), self);

    remove_action = g_simple_action_new ("remove", NULL);
    g_signal_connect (remove_action, "activate", G_CALLBACK (uninstall), self);

    g_action_map_add_action (G_ACTION_MAP (self->action_group), G_ACTION (state_action));
    g_action_map_add_action (G_ACTION_MAP (self->action_group), G_ACTION (open_prefs_action));
    g_action_map_add_action (G_ACTION_MAP (self->action_group), G_ACTION (remove_action));

    gtk_widget_insert_action_group (GTK_WIDGET (self), "row", G_ACTION_GROUP (self->action_group));
}
