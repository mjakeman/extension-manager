/*
 * exm-extension-row.c
 *
 * Copyright 2022-2025 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#include "exm-config.h"
#include "exm-enums.h"
#include "exm-types.h"

#include <glib/gi18n.h>

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

    AdwActionRow *description_row;
    AdwActionRow *version_row;
    AdwActionRow *session_modes_row;
    GtkLabel *info_label;
    AdwActionRow *error_row;

    GtkImage *update_icon;
    GtkImage *error_icon;
    GtkImage *out_of_date_icon;
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

static gboolean
transform_to_state (GBinding     *binding G_GNUC_UNUSED,
                    const GValue *from_value,
                    GValue       *to_value,
                    gpointer      user_data G_GNUC_UNUSED)
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
add_session_modes (GPtrArray       *session_modes,
                   ExmExtensionRow *self)
{
    if (!session_modes || session_modes->len == 0)
        return;

    GPtrArray *subtitles = g_ptr_array_new_with_free_func (g_free);
    gboolean has_unlock_dialog = FALSE;
    gboolean has_gdm = FALSE;

    for (guint i = 0; i < session_modes->len; i++)
    {
        gchar *mode = g_ptr_array_index (session_modes, i);

        if (g_strcmp0 (mode, "unlock-dialog") == 0)
        {
            g_ptr_array_add (subtitles, g_strdup (_("Unlock Dialog")));
            has_unlock_dialog = TRUE;
        }
        else if (g_strcmp0 (mode, "gdm") == 0)
        {
            // Translators: GNOME Display Manager
            g_ptr_array_add (subtitles, g_strdup (_("GDM")));
            has_gdm = TRUE;
        }
    }

    if (has_unlock_dialog && has_gdm)
    {
        // Translators: Label when an extension runs on both, login and lock screens
        gtk_label_set_label (self->info_label, _("This extension will run while the screen is locked and no user is logged in"));
    }
    else if (has_unlock_dialog)
    {
        // Translators: Label when an extension runs on the lock screen
        gtk_label_set_label (self->info_label, _("This extension will run while the screen is locked"));
    }
    else if (has_gdm)
    {
        // Translators: Label when an extension runs on the login screen
        gtk_label_set_label (self->info_label, _("This extension will run while no user is logged in"));
    }

    if (subtitles->len > 0)
    {
        g_ptr_array_add (subtitles, NULL);
        gchar *subtitle = g_strjoinv (" / ", (gchar **)subtitles->pdata);
        adw_action_row_set_subtitle (self->session_modes_row, subtitle);
        gtk_widget_set_visible (GTK_WIDGET (self->session_modes_row), TRUE);
        g_free (subtitle);
    }

    g_ptr_array_free (subtitles, TRUE);
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

    gchar *name, *uuid, *description, *version, *version_name, *error;
    gboolean enabled, has_prefs, is_user;
    ExmExtensionState state;
    GPtrArray *session_modes;
    g_object_get (self->extension,
                  "name", &name,
                  "uuid", &uuid,
                  "description", &description,
                  "state", &state,
                  "enabled", &enabled,
                  "version", &version,
                  "version-name", &version_name,
                  "error", &error,
                  "has-prefs", &has_prefs,
                  "is-user", &is_user,
                  "session-modes", &session_modes,
                  NULL);

    self->uuid = g_strdup (uuid);

    g_object_set (self, "title", g_markup_escape_text (name, -1), NULL);

    // Trim description label's leading and trailing whitespace
    char *description_trimmed = g_strchomp (g_strstrip (description));
    adw_action_row_set_subtitle (self->description_row, description_trimmed);
    g_free (description_trimmed);

    // Only show if error exists and is not empty
    gboolean has_error = (error != NULL) && (strlen (error) != 0);
    gtk_widget_set_visible (GTK_WIDGET (self->error_row), has_error);

    gtk_widget_set_visible (GTK_WIDGET (self->error_icon), state == EXM_EXTENSION_STATE_ERROR);
    gtk_widget_set_visible (GTK_WIDGET (self->out_of_date_icon), state == EXM_EXTENSION_STATE_OUT_OF_DATE);

    gtk_widget_set_visible (GTK_WIDGET (self->version_row), version != NULL);
    adw_action_row_set_subtitle (self->version_row, version_name ? version_name
                                                                 : version);

    gtk_actionable_set_action_target (GTK_ACTIONABLE (self->details_btn), "s", uuid);

    add_session_modes (session_modes, self);

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

void
exm_search_row_focus_toggle (ExmExtensionRow *self)
{
    gtk_widget_grab_focus (GTK_WIDGET (self->ext_toggle));
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

    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, description_row);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, version_row);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, session_modes_row);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, info_label);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, error_row);

    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, prefs_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, remove_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, details_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, ext_toggle);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, update_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, error_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, out_of_date_icon);

    gtk_widget_class_bind_template_callback (widget_class, on_state_changed);
}

static void
open_prefs (GSimpleAction   *action G_GNUC_UNUSED,
            GVariant        *new_value G_GNUC_UNUSED,
            ExmExtensionRow *self)
{
    g_return_if_fail (self->extension);

    gtk_widget_activate_action (GTK_WIDGET (self),
                                "ext.open-prefs",
                                "s", self->uuid);
}

static void
uninstall (GSimpleAction   *action G_GNUC_UNUSED,
           GVariant        *new_value G_GNUC_UNUSED,
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
