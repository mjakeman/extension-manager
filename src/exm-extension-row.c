/* exm-extension-row.c
 *
 * Copyright 2022-2024 Matthew Jakeman <mjakeman26@outlook.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "exm-extension-row.h"

#include "exm-enums.h"
#include "exm-types.h"

#include "exm-config.h"

struct _ExmExtensionRow
{
    AdwExpanderRow parent_instance;

    GSimpleActionGroup *action_group;

    ExmExtension *extension;
    gchar *uuid;

    ExmManager *manager;

    GtkButton *remove_btn;
    GtkButton *prefs_btn;
    GtkButton *details_btn;
    GtkSwitch *ext_toggle;

    GtkLabel *description_label;
    GtkLabel *version_title;
    GtkLabel *version_label;
    GtkLabel *error_label;
    GtkLabel *error_label_tag;

    GtkImage *update_icon;
    GtkImage *error_icon;
    GtkImage *out_of_date_icon;
    GtkImage *info_icon;
};

G_DEFINE_FINAL_TYPE (ExmExtensionRow, exm_extension_row, ADW_TYPE_EXPANDER_ROW)

enum {
    PROP_0,
    PROP_EXTENSION,
    PROP_MANAGER,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
bind_extension (ExmExtensionRow *self,
                ExmExtension    *extension);

static void
unbind_extension (ExmExtensionRow *self);

ExmExtensionRow *
exm_extension_row_new (ExmExtension *extension,
                       ExmManager   *manager)
{
    return g_object_new (EXM_TYPE_EXTENSION_ROW,
                         "extension", extension,
                         "manager", manager,
                         NULL);
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
    case PROP_MANAGER:
        g_value_set_object (value, self->manager);
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
    case PROP_MANAGER:
        self->manager = g_value_get_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
set_error_label_visible (ExmExtensionRow *self,
                         gboolean         visible)
{
    gtk_widget_set_visible (GTK_WIDGET (self->error_label), visible);
    gtk_widget_set_visible (GTK_WIDGET (self->error_label_tag), visible);
}

static gboolean
transform_to_state (GBinding     *binding,
                    const GValue *from_value,
                    GValue       *to_value,
                    gpointer      user_data)
{
    g_value_set_boolean (to_value, g_value_get_enum (from_value) == EXM_EXTENSION_STATE_ACTIVE);

    return TRUE;
}

static void
unbind_extension (ExmExtensionRow *self)
{
    if (self->extension != NULL)
    {
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

    gchar *name, *uuid, *description, *version, *version_name, *error_msg;
    gboolean enabled, has_prefs, has_update, is_user;
    ExmExtensionState state;
    g_object_get (self->extension,
                  "display-name", &name,
                  "uuid", &uuid,
                  "description", &description,
                  "state", &state,
                  "enabled", &enabled,
                  "has-prefs", &has_prefs,
                  "has-update", &has_update,
                  "is-user", &is_user,
                  "version", &version,
                  "version-name", &version_name,
                  "error-msg", &error_msg,
                  NULL);

    self->uuid = g_strdup (uuid);

    g_object_set (self, "title", g_markup_escape_text(name, -1), "subtitle", uuid, NULL);
    g_object_set (self->prefs_btn, "visible", has_prefs, NULL);
    g_object_set (self->remove_btn, "visible", is_user, NULL);
    g_object_set (self->update_icon, "visible", has_update, NULL);
    g_object_set (self->version_label, "label", version_name ? g_strdup_printf ("%s (%s)", version_name, version)
                                                             : version, NULL);

    // Trim description label's leading and trailing whitespace
    char *description_trimmed = g_strchomp (g_strstrip (description));
    g_object_set (self->description_label, "label", description_trimmed, NULL);
    g_free (description_trimmed);

    // Only show if error_msg exists and is not empty
    g_object_set (self->error_label, "label", error_msg, NULL);
    gboolean has_error = (error_msg != NULL) && (strlen(error_msg) != 0);
    set_error_label_visible (self, has_error);

    gtk_widget_set_visible (GTK_WIDGET (self->error_icon), state == EXM_EXTENSION_STATE_ERROR);
    gtk_widget_set_visible (GTK_WIDGET (self->out_of_date_icon), state == EXM_EXTENSION_STATE_OUT_OF_DATE);

    gtk_widget_set_visible (GTK_WIDGET (self->info_icon),
                            (state == EXM_EXTENSION_STATE_INITIALIZED
                             || state == EXM_EXTENSION_STATE_INACTIVE)
                             && enabled);

    gtk_widget_set_visible (GTK_WIDGET (self->version_title), version != NULL);
    gtk_widget_set_visible (GTK_WIDGET (self->version_label), version != NULL);

    gtk_actionable_set_action_target (GTK_ACTIONABLE (self->details_btn), "s", uuid);

    GAction *action;

    action = g_action_map_lookup_action (G_ACTION_MAP (self->action_group), "open-prefs");
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_prefs);

    action = g_action_map_lookup_action (G_ACTION_MAP (self->action_group), "remove");
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action), is_user);

    g_object_bind_property_full (self->extension,
                                 "state",
                                 self->ext_toggle,
                                 "state",
                                 G_BINDING_SYNC_CREATE,
                                 transform_to_state,
                                 NULL,
                                 NULL,
                                 NULL);

    // Keep compatibility with GNOME Shell versions prior to 46
    if (gtk_switch_get_state (self->ext_toggle) != enabled &&
        (state == EXM_EXTENSION_STATE_ACTIVE || state == EXM_EXTENSION_STATE_ACTIVATING))
        g_object_set (self->extension, "enabled", !enabled, NULL);
}

static gboolean
on_state_changed (GtkSwitch        *toggle,
                  gboolean          state,
                  ExmExtensionRow  *self)
{
    g_return_val_if_fail (EXM_IS_EXTENSION_ROW (self), FALSE);

    g_assert (self->ext_toggle == toggle);

    gboolean enabled;

    g_object_get (self->extension, "enabled", &enabled, NULL);

    // Prevents changing extensions' state when global switch is toggled
    if (state == enabled)
        return TRUE;

    // Keep compatibility with GNOME Shell versions prior to 46
    if (gtk_switch_get_state (toggle) != enabled)
        g_object_set (self->extension, "enabled", !enabled, NULL);

    if (state)
        exm_manager_enable_extension (self->manager, self->extension);
    else
        exm_manager_disable_extension (self->manager, self->extension);

    return TRUE;
}

static void
exm_extension_row_class_init (ExmExtensionRowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = exm_extension_row_dispose;
    object_class->get_property = exm_extension_row_get_property;
    object_class->set_property = exm_extension_row_set_property;

    properties [PROP_EXTENSION] =
        g_param_spec_object ("extension",
                             "Extension",
                             "Extension",
                             EXM_TYPE_EXTENSION,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_MANAGER]
        = g_param_spec_object ("manager",
                               "Manager",
                               "Manager",
                               EXM_TYPE_MANAGER,
                               G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-extension-row.ui", RESOURCE_PATH));

    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, description_label);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, version_title);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, version_label);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, error_label);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, error_label_tag);

    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, prefs_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, remove_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, details_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, ext_toggle);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, update_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, error_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, out_of_date_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, info_icon);

    gtk_widget_class_bind_template_callback (widget_class, on_state_changed);
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
    GSimpleAction *open_prefs_action;
    GSimpleAction *remove_action;

    gtk_widget_init_template (GTK_WIDGET (self));

    // Define Actions
    self->action_group = g_simple_action_group_new ();

    open_prefs_action = g_simple_action_new ("open-prefs", NULL);
    g_signal_connect (open_prefs_action, "activate", G_CALLBACK (open_prefs), self);

    remove_action = g_simple_action_new ("remove", NULL);
    g_signal_connect (remove_action, "activate", G_CALLBACK (uninstall), self);

    g_action_map_add_action (G_ACTION_MAP (self->action_group), G_ACTION (open_prefs_action));
    g_action_map_add_action (G_ACTION_MAP (self->action_group), G_ACTION (remove_action));

    gtk_widget_insert_action_group (GTK_WIDGET (self), "row", G_ACTION_GROUP (self->action_group));
}
