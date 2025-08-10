/*
 * exm-info-bar.c
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

#include "exm-info-bar.h"

#include "exm-config.h"

#include <adwaita.h>
#include <glib/gi18n.h>

struct _ExmInfoBar
{
    GtkBox parent_instance;

    GtkListBox *session_modes_item;

    gint downloads;
    gchar *version;
    gchar **session_modes;
};

G_DEFINE_FINAL_TYPE (ExmInfoBar, exm_info_bar, GTK_TYPE_BOX)

enum {
    PROP_0,
    PROP_DOWNLOADS,
    PROP_VERSION,
    PROP_SESSION_MODES,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmInfoBar *
exm_info_bar_new (void)
{
    return g_object_new (EXM_TYPE_INFO_BAR, NULL);
}

static void
exm_info_bar_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    ExmInfoBar *self = EXM_INFO_BAR (object);

    switch (prop_id)
    {
    case PROP_DOWNLOADS:
        g_value_set_int (value, self->downloads);
        break;
    case PROP_VERSION:
        g_value_set_string (value, self->version);
        break;
    case PROP_SESSION_MODES:
        g_value_set_boxed (value, self->session_modes);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
update_session_modes_visibility (ExmInfoBar *self);

static void
exm_info_bar_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    ExmInfoBar *self = EXM_INFO_BAR (object);

    switch (prop_id)
    {
    case PROP_DOWNLOADS:
        self->downloads = g_value_get_int (value);
        break;
    case PROP_VERSION:
        self->version = g_value_dup_string (value);
        break;
    case PROP_SESSION_MODES:
        self->session_modes = g_value_get_boxed (value);
        update_session_modes_visibility (self);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
update_session_modes_visibility (ExmInfoBar *self)
{
    gtk_widget_set_visible (GTK_WIDGET (self->session_modes_item),
                            self->session_modes != NULL && self->session_modes[0] != NULL);
}

static gchar*
format_downloads (GObject *object G_GNUC_UNUSED,
                  gint     downloads)
{
    return g_strdup_printf ("%'d", downloads);
}

static gchar*
format_session_modes (GObject  *object G_GNUC_UNUSED,
                      gchar   **session_modes)
{
    return (session_modes && g_strcmp0 (session_modes[0], "unlock-dialog") == 0)
           ? g_strdup (_("Unlock Dialog"))
           : g_strdup ("");
}

static void
exm_info_bar_class_init (ExmInfoBarClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = exm_info_bar_get_property;
    object_class->set_property = exm_info_bar_set_property;

    properties [PROP_DOWNLOADS]
        = g_param_spec_int ("downloads",
                            "Downloads",
                            "Downloads",
                            0, G_MAXINT, 0,
                            G_PARAM_READWRITE);

    properties [PROP_VERSION]
        = g_param_spec_string ("version",
                               "Version",
                               "Version",
                               NULL,
                               G_PARAM_READWRITE);

    properties[PROP_SESSION_MODES] =
        g_param_spec_boxed ("session_modes",
                            "Session Modes",
                            "Session Modes",
                            G_TYPE_STRV,
                            G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-info-bar.ui", RESOURCE_PATH));

    gtk_widget_class_bind_template_child (widget_class, ExmInfoBar, session_modes_item);

    gtk_widget_class_bind_template_callback (widget_class, format_downloads);
    gtk_widget_class_bind_template_callback (widget_class, format_session_modes);

    gtk_widget_class_set_css_name (widget_class, "info-bar");
}

static void
exm_info_bar_init (ExmInfoBar *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}
