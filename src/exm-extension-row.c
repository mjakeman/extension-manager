#include "exm-extension-row.h"

#include "exm-enums.h"
#include "exm-types.h"

struct _ExmExtensionRow
{
    AdwExpanderRow parent_instance;

    GSimpleActionGroup *action_group;

    ExmExtension *extension;
    gchar *uuid;

    GtkButton *remove_btn;
    GtkButton *prefs_btn;
    GtkButton *details_btn;
    GtkSwitch *ext_toggle;

    GtkLabel *description_label;
    GtkLabel *version_label;
    GtkLabel *error_label;
    GtkLabel *error_label_tag;

    GtkImage *update_icon;
    GtkImage *error_icon;
    GtkImage *out_of_date_icon;

    guint signal_handler;
};

G_DEFINE_FINAL_TYPE (ExmExtensionRow, exm_extension_row, ADW_TYPE_EXPANDER_ROW)

enum {
    PROP_0,
    PROP_EXTENSION,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
bind_extension (ExmExtensionRow *self,
                ExmExtension    *extension);

static void
unbind_extension (ExmExtensionRow *self);

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
exm_extension_row_dispose (GObject *object)
{
    ExmExtensionRow *self = (ExmExtensionRow *)object;

    unbind_extension (self);

    G_OBJECT_CLASS (exm_extension_row_parent_class)->dispose (object);
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
        bind_extension (self, g_value_get_object (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

void
update_state (ExmExtension    *extension,
              GParamSpec      *pspec,
              ExmExtensionRow *row)
{
    // We update the state of the action without activating it. If we activate
    // it, then it will go back to gnome-shell and explicitly enable/disable
    // the extension. We do not want this behaviour as it messes with the global
    // extension toggle.

    g_return_if_fail (EXM_IS_EXTENSION (extension));
    g_return_if_fail (EXM_IS_EXTENSION_ROW (row));

    g_assert (row->extension == extension);

    const gchar *uuid;
    ExmExtensionState new_state;
    GAction *action;

    g_object_get (extension,
                  "state", &new_state,
                  "uuid", &uuid,
                  NULL);

    g_info ("%s: %s\n", uuid, g_enum_to_string (EXM_TYPE_EXTENSION_STATE, new_state));

    action = g_action_map_lookup_action (G_ACTION_MAP (row->action_group), "state-set");

    // Reset state
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action), TRUE);
    gtk_widget_set_visible (GTK_WIDGET (row->error_icon), FALSE);
    gtk_widget_set_visible (GTK_WIDGET (row->out_of_date_icon), FALSE);

    switch (new_state)
    {
    case EXM_EXTENSION_STATE_ENABLED:
        g_simple_action_set_state (G_SIMPLE_ACTION (action),
                                   g_variant_new_boolean (TRUE));
        break;

    case EXM_EXTENSION_STATE_DISABLED:
        g_simple_action_set_state (G_SIMPLE_ACTION (action),
                                   g_variant_new_boolean (FALSE));
        break;

    case EXM_EXTENSION_STATE_ERROR:
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
        gtk_widget_set_visible (GTK_WIDGET (row->error_icon), TRUE);
        g_simple_action_set_state (G_SIMPLE_ACTION (action),
                                   g_variant_new_boolean (FALSE));
        break;

    case EXM_EXTENSION_STATE_OUT_OF_DATE:
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
        gtk_widget_set_visible (GTK_WIDGET (row->out_of_date_icon), TRUE);
        g_simple_action_set_state (G_SIMPLE_ACTION (action),
                                   g_variant_new_boolean (FALSE));
        break;

    default:
        break;
    }
    gboolean is_enabled = (new_state == EXM_EXTENSION_STATE_ENABLED);

    // Update state of toggle
    g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (is_enabled));
}

static void
set_error_label_visible (ExmExtensionRow *self,
                         gboolean         visible)
{
    gtk_widget_set_visible (GTK_WIDGET (self->error_label), visible);
    gtk_widget_set_visible (GTK_WIDGET (self->error_label_tag), visible);
}

static void
unbind_extension (ExmExtensionRow *self)
{
    if (self->extension != NULL)
    {
        g_signal_handler_disconnect (self->extension, self->signal_handler);
        g_clear_object (&self->extension);
        g_clear_pointer (&self->uuid, g_free);
    }
}

static void
bind_extension (ExmExtensionRow *self,
                ExmExtension    *extension)
{
    // TODO: This big block of property assignments is currently copy/pasted
    // from ExmExtension. We can replace this with GtkExpression lookups
    // once blueprint-compiler supports expressions.
    // (See https://gitlab.gnome.org/jwestman/blueprint-compiler/-/issues/5)

    g_return_if_fail (EXM_IS_EXTENSION_ROW (self));

    // First, remove traces of the old extension
    unbind_extension (self);

    // Now, bind the new one
    self->extension = g_object_ref (extension);

    if (self->extension == NULL)
        return;

    gchar *name, *uuid, *description, *version, *error_msg;
    gboolean has_prefs, has_update, is_user;
    ExmExtensionState state;
    g_object_get (self->extension,
                  "display-name", &name,
                  "uuid", &uuid,
                  "description", &description,
                  "state", &state,
                  "has-prefs", &has_prefs,
                  "has-update", &has_update,
                  "is-user", &is_user,
                  "version", &version,
                  "error-msg", &error_msg,
                  NULL);

    self->uuid = g_strdup (uuid);

    g_object_set (self, "title", g_markup_escape_text(name, -1), "subtitle", uuid, NULL);
    g_object_set (self->prefs_btn, "visible", has_prefs, NULL);
    g_object_set (self->remove_btn, "visible", is_user, NULL);
    g_object_set (self->update_icon, "visible", has_update, NULL);
    g_object_set (self->version_label, "label", version, NULL);

    // Trim description label's leading and trailing whitespace
    char *description_trimmed = g_strchomp (g_strstrip (description));
    g_object_set (self->description_label, "label", description_trimmed, NULL);
    g_free (description_trimmed);

    // Only show if error_msg exists and is not empty
    g_object_set (self->error_label, "label", error_msg, NULL);
    gboolean has_error = (error_msg != NULL) && (strlen(error_msg) != 0);
    set_error_label_visible (self, has_error);

    gtk_actionable_set_action_target (GTK_ACTIONABLE (self->details_btn), "s", uuid);

    // One way binding from extension ("source of truth") to switch
    self->signal_handler = g_signal_connect (self->extension,
                                             "notify::state",
                                             G_CALLBACK (update_state),
                                             self);

    GAction *action;

    action = g_action_map_lookup_action (G_ACTION_MAP (self->action_group), "state-set");
    g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (TRUE));

    action = g_action_map_lookup_action (G_ACTION_MAP (self->action_group), "open-prefs");
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_prefs);

    action = g_action_map_lookup_action (G_ACTION_MAP (self->action_group), "remove");
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action), is_user);

    update_state (self->extension, NULL, self);
}

static void
exm_extension_row_class_init (ExmExtensionRowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_extension_row_finalize;
    object_class->dispose = exm_extension_row_dispose;
    object_class->get_property = exm_extension_row_get_property;
    object_class->set_property = exm_extension_row_set_property;

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
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, error_label);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, error_label_tag);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, version_label);

    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, prefs_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, remove_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, details_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, ext_toggle);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, update_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, error_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, out_of_date_icon);
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
    GtkWidget *action_row;

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

