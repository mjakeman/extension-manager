/*
 * exm-extension-row.c
 *
 * Copyright 2022 Matthew Jakeman <mjakeman26@outlook.co.nz>
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
#include "exm-extension-toggle.h"

#include <glib/gi18n.h>

struct _ExmExtensionRow
{
    AdwActionRow parent_instance;

    GSimpleActionGroup *action_group;

    ExmExtension *extension;
    gchar *uuid;

    ExmManager *manager;

    GtkCheckButton *select_check;
    GtkButton *prefs_btn;
    GtkSwitch *ext_toggle;
    GtkImage *nav_icon;

    GtkImage *update_icon;
    GtkButton *error_btn;
    GtkImage *out_of_date_icon;

    gboolean selection_mode;
};

G_DEFINE_FINAL_TYPE (ExmExtensionRow, exm_extension_row, ADW_TYPE_ACTION_ROW)

enum {
    PROP_0,
    PROP_EXTENSION,
    PROP_MANAGER,
    PROP_SELECTION_MODE,
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

    g_clear_object (&self->select_check);

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
    case PROP_SELECTION_MODE:
        g_value_set_boolean (value, self->selection_mode);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
row_is_selectable (ExmExtensionRow *self)
{
    gboolean is_user = FALSE;

    if (self->extension)
        g_object_get (self->extension, "is-user", &is_user, NULL);

    return is_user;
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
    case PROP_SELECTION_MODE:
        {
            gboolean page_selection_mode = g_value_get_boolean (value);
            gboolean is_selectable = row_is_selectable (self);
            gboolean new_mode = page_selection_mode && is_selectable;
            GtkWidget *prefix_box;

            self->selection_mode = new_mode;

            gtk_widget_set_visible (GTK_WIDGET (self), !page_selection_mode || is_selectable);

            if (gtk_widget_get_parent (GTK_WIDGET (self->select_check)) == NULL)
            {
                if (new_mode)
                    adw_action_row_add_prefix (ADW_ACTION_ROW (self),
                                               GTK_WIDGET (self->select_check));
            }
            else
            {
                prefix_box = gtk_widget_get_parent (GTK_WIDGET (self->select_check));
                if (!new_mode)
                    gtk_check_button_set_active (self->select_check, FALSE);
                gtk_widget_set_visible (GTK_WIDGET (self->select_check), new_mode);
                gtk_widget_set_visible (prefix_box, new_mode);
            }

            if (self->prefs_btn)
            {
                gboolean show_prefs = FALSE;
                if (!new_mode && self->extension)
                    g_object_get (self->extension, "has-prefs", &show_prefs, NULL);
                gtk_widget_set_visible (GTK_WIDGET (self->prefs_btn), show_prefs);
            }
            if (self->ext_toggle)
                gtk_widget_set_visible (GTK_WIDGET (self->ext_toggle), !new_mode);
            if (self->nav_icon)
                gtk_widget_set_visible (GTK_WIDGET (self->nav_icon), !new_mode);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
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
    g_return_if_fail (EXM_IS_EXTENSION_ROW (self));

    // First, remove traces of the old extension
    unbind_extension (self);

    // Now, bind the new one
    self->extension = g_object_ref (extension);

    if (self->extension == NULL)
        return;

    gchar *uuid;
    gboolean has_prefs;
    g_object_get (self->extension,
                  "uuid", &uuid,
                  "has-prefs", &has_prefs,
                  NULL);

    self->uuid = g_strdup (uuid);

    GAction *action = g_action_map_lookup_action (G_ACTION_MAP (self->action_group), "open-prefs");
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_prefs);

    exm_extension_bind_toggle (self->extension, self->ext_toggle);
}

static gboolean
on_state_changed (GtkSwitch       *toggle,
                  gboolean         state,
                  ExmExtensionRow *self)
{
    g_return_val_if_fail (EXM_IS_EXTENSION_ROW (self), FALSE);

    if (!self->manager || !self->extension)
        return FALSE;

    return exm_extension_apply_toggle (self->manager, self->extension, toggle, state);
}

void
exm_search_row_focus_toggle (ExmExtensionRow *self)
{
    gtk_widget_grab_focus (GTK_WIDGET (self->ext_toggle));
}

static void
on_select_toggled (GtkCheckButton  *check G_GNUC_UNUSED,
                   ExmExtensionRow *self)
{
    if (!self->selection_mode)
        return;
    if (self->uuid)
        gtk_widget_activate_action (GTK_WIDGET (self), "page.selection-changed", "(sb)",
                                    self->uuid,
                                    gtk_check_button_get_active (self->select_check));
}

static void
on_row_activated (ExmExtensionRow *self)
{
    if (self->selection_mode)
    {
        gtk_check_button_set_active (self->select_check,
                                     !gtk_check_button_get_active (self->select_check));
        return;
    }
    if (self->uuid)
        gtk_widget_activate_action (GTK_WIDGET (self), "win.show-detail", "s", self->uuid);
}

static gboolean
on_state_is_error (ExmExtensionRow  *self G_GNUC_UNUSED,
                   ExmExtensionState state)
{
    return state == EXM_EXTENSION_STATE_ERROR;
}

static gboolean
on_state_is_out_of_date (ExmExtensionRow  *self G_GNUC_UNUSED,
                         ExmExtensionState state)
{
    return state == EXM_EXTENSION_STATE_OUT_OF_DATE;
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

    properties [PROP_SELECTION_MODE]
        = g_param_spec_boolean ("selection-mode",
                                "Selection Mode",
                                "Whether the row is in multi-select mode",
                                FALSE,
                                G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-extension-row.ui", RESOURCE_PATH));

    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, prefs_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, ext_toggle);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, nav_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, update_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, error_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmExtensionRow, out_of_date_icon);

    gtk_widget_class_bind_template_callback (widget_class, on_state_changed);
    gtk_widget_class_bind_template_callback (widget_class, on_row_activated);
    gtk_widget_class_bind_template_callback (widget_class, on_state_is_error);
    gtk_widget_class_bind_template_callback (widget_class, on_state_is_out_of_date);
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
show_error (GSimpleAction   *action G_GNUC_UNUSED,
           GVariant        *new_value G_GNUC_UNUSED,
           ExmExtensionRow *self)
{
    g_return_if_fail (self->extension);

    gtk_widget_activate_action (GTK_WIDGET (self),
                                "ext.show-error",
                                "s", self->uuid);
}

static void
exm_extension_row_init (ExmExtensionRow *self)
{
    GSimpleAction *open_prefs_action;
    GSimpleAction *show_error_action;

    gtk_widget_init_template (GTK_WIDGET (self));

    gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (self), TRUE);

    self->select_check = GTK_CHECK_BUTTON (gtk_check_button_new ());
    gtk_widget_set_valign (GTK_WIDGET (self->select_check), GTK_ALIGN_CENTER);
    gtk_widget_add_css_class (GTK_WIDGET (self->select_check), "selection-mode");
    g_object_ref_sink (self->select_check);
    g_signal_connect (self->select_check, "toggled",
                      G_CALLBACK (on_select_toggled), self);

    // Define Actions
    self->action_group = g_simple_action_group_new ();

    open_prefs_action = g_simple_action_new ("open-prefs", NULL);
    g_signal_connect (open_prefs_action, "activate", G_CALLBACK (open_prefs), self);

    show_error_action = g_simple_action_new ("show-error", NULL);
    g_signal_connect (show_error_action, "activate", G_CALLBACK (show_error), self);

    g_action_map_add_action (G_ACTION_MAP (self->action_group), G_ACTION (open_prefs_action));
    g_action_map_add_action (G_ACTION_MAP (self->action_group), G_ACTION (show_error_action));

    gtk_widget_insert_action_group (GTK_WIDGET (self), "row", G_ACTION_GROUP (self->action_group));
}

gboolean
exm_extension_row_is_selected (ExmExtensionRow *self)
{
    g_return_val_if_fail (EXM_IS_EXTENSION_ROW (self), FALSE);
    return gtk_check_button_get_active (self->select_check);
}

void
exm_extension_row_set_selected (ExmExtensionRow *self,
                                gboolean         selected)
{
    g_return_if_fail (EXM_IS_EXTENSION_ROW (self));

    if (!self->selection_mode)
        return;

    // The page is the source of truth, so don't report this change back
    g_signal_handlers_block_by_func (self->select_check, on_select_toggled, self);
    gtk_check_button_set_active (self->select_check, selected);
    g_signal_handlers_unblock_by_func (self->select_check, on_select_toggled, self);
}
