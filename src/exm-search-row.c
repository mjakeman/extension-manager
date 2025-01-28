/* exm-search-row.c
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

#include "exm-search-row.h"

#include "exm-install-button.h"

#include "exm-types.h"
#include "exm-enums.h"

#include "exm-config.h"

#include <glib/gi18n.h>

struct _ExmSearchRow
{
    GtkListBoxRow parent_instance;

    ExmSearchResult *search_result;
    gboolean is_installed;
    gboolean is_supported;
    gchar *uuid;

    GtkLabel *description_label;
    ExmInstallButton *install_btn;
};

G_DEFINE_FINAL_TYPE (ExmSearchRow, exm_search_row, GTK_TYPE_LIST_BOX_ROW)

enum {
    PROP_0,
    PROP_SEARCH_RESULT,
    PROP_IS_INSTALLED,
    PROP_IS_SUPPORTED,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmSearchRow *
exm_search_row_new (ExmSearchResult *search_result,
                    gboolean         is_installed,
                    gboolean         is_supported)
{
    return g_object_new (EXM_TYPE_SEARCH_ROW,
                         "search-result", search_result,
                         "is-installed", is_installed,
                         "is-supported", is_supported,
                         NULL);
}

static void
exm_search_row_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
    ExmSearchRow *self = EXM_SEARCH_ROW (object);

    switch (prop_id)
    {
    case PROP_SEARCH_RESULT:
        g_value_set_object (value, self->search_result);
        break;
    case PROP_IS_INSTALLED:
        g_value_set_boolean (value, self->is_installed);
        break;
    case PROP_IS_SUPPORTED:
        g_value_set_boolean (value, self->is_supported);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_search_row_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
    ExmSearchRow *self = EXM_SEARCH_ROW (object);

    switch (prop_id)
    {
    case PROP_SEARCH_RESULT:
        self->search_result = g_value_get_object (value);
        if (self->search_result)
        {
            // TODO: Bind here, rather than in constructed()
            g_object_get (self->search_result,
                          "uuid", &self->uuid,
                          NULL);
        }
        break;
    case PROP_IS_INSTALLED:
        self->is_installed = g_value_get_boolean (value);
        break;
    case PROP_IS_SUPPORTED:
        self->is_supported = g_value_get_boolean (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
install_remote (GtkButton    *button,
                ExmSearchRow *self)
{
    gboolean warn;
    ExmInstallButtonState state;

    g_object_get (self->install_btn, "state", &state, NULL);

    warn = (state == EXM_INSTALL_BUTTON_STATE_UNSUPPORTED);
    gtk_widget_activate_action (GTK_WIDGET (button),
                                "ext.install",
                                "(sb)", self->uuid, warn);
}

static void
exm_search_row_constructed (GObject *object)
{
    ExmSearchRow *self = EXM_SEARCH_ROW (object);

    ExmInstallButtonState install_state;

    gchar *uuid, *name, *creator, *description;
    g_object_get (self->search_result,
                  "uuid", &uuid,
                  "name", &name,
                  "creator", &creator,
                  "description", &description,
                  NULL);

    gtk_actionable_set_action_target (GTK_ACTIONABLE (self), "s", uuid);

    install_state = self->is_installed
        ? EXM_INSTALL_BUTTON_STATE_INSTALLED
        : (self->is_supported
           ? EXM_INSTALL_BUTTON_STATE_DEFAULT
           : EXM_INSTALL_BUTTON_STATE_UNSUPPORTED);

    g_object_set (self->install_btn, "state", install_state, NULL);

    gtk_accessible_update_property (GTK_ACCESSIBLE (self),
                                    // Translators: '%s' = extension name, '%s' = extension creator
                                    GTK_ACCESSIBLE_PROPERTY_LABEL, g_strdup_printf (_("%s by %s"), name, creator),
                                    -1);

    const gchar *newline_pos = g_strstr_len (description, -1, "\n");

    if (newline_pos != NULL)
    {
        gchar *truncated_text = g_strndup (description, newline_pos - description);
        gtk_label_set_label (self->description_label, truncated_text);
        g_free (truncated_text);
    }
    else
    {
        gtk_label_set_label (self->description_label, description);
    }

    G_OBJECT_CLASS (exm_search_row_parent_class)->constructed (object);
}

static void
exm_search_row_class_init (ExmSearchRowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = exm_search_row_get_property;
    object_class->set_property = exm_search_row_set_property;
    object_class->constructed = exm_search_row_constructed;

    properties [PROP_SEARCH_RESULT] =
        g_param_spec_object ("search-result",
                             "Search Result",
                             "Search Result",
                             EXM_TYPE_SEARCH_RESULT,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_IS_INSTALLED] =
        g_param_spec_boolean ("is-installed",
                              "Is Installed",
                              "Is Installed",
                              FALSE,
                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_IS_SUPPORTED] =
        g_param_spec_boolean ("is-supported",
                              "Is Supported",
                              "Is Supported",
                              FALSE,
                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-search-row.ui", RESOURCE_PATH));

    gtk_widget_class_bind_template_child (widget_class, ExmSearchRow, description_label);
    gtk_widget_class_bind_template_child (widget_class, ExmSearchRow, install_btn);

    gtk_widget_class_bind_template_callback (widget_class, install_remote);
}

static void
exm_search_row_init (ExmSearchRow *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}
