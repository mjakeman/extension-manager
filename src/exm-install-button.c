/* exm-install-button.c
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

#include "exm-install-button.h"

#include "exm-types.h"
#include "exm-enums.h"

#include <glib/gi18n.h>

struct _ExmInstallButton
{
    GtkButton parent_instance;

    ExmInstallButtonState state;
};

G_DEFINE_FINAL_TYPE (ExmInstallButton, exm_install_button, GTK_TYPE_BUTTON)

enum {
    PROP_0,
    PROP_STATE,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
update_state (ExmInstallButton *button);

ExmInstallButton *
exm_install_button_new (void)
{
    return g_object_new (EXM_TYPE_INSTALL_BUTTON, NULL);
}

static void
exm_install_button_finalize (GObject *object)
{
    ExmInstallButton *self = (ExmInstallButton *)object;

    G_OBJECT_CLASS (exm_install_button_parent_class)->finalize (object);
}

static void
exm_install_button_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
    ExmInstallButton *self = EXM_INSTALL_BUTTON (object);

    switch (prop_id)
    {
    case PROP_STATE:
        g_value_set_enum (value, self->state);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_install_button_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
    ExmInstallButton *self = EXM_INSTALL_BUTTON (object);

    switch (prop_id)
    {
    case PROP_STATE:
        self->state = g_value_get_enum (value);
        update_state (self);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
is_version_check_disabled ()
{
    // Unfortunately we do not have access to the
    // `disable-extension-version-validation` gsettings
    // key under flatpak due to the sandbox.
    return FALSE;

    /*GSettingsSchemaSource *source;
    GSettingsSchema *schema;
    GSettings *settings;
    const gchar *key_name;

    key_name = "disable-extension-version-validation";

    source = g_settings_schema_source_get_default ();
    schema = g_settings_schema_source_lookup (source, "org.gnome.shell", TRUE);

    if (schema && g_settings_schema_has_key (schema, key_name))
    {
        settings = g_settings_new ("org.gnome.shell");
        return g_settings_get_boolean (settings, "disable-extension-version-validation");
    }

    g_warning ("Could not find schema `org.gnome.shell` with key `%s`.", key_name);

    return FALSE;*/
}

static void
update_state (ExmInstallButton *button)
{
    const gchar *tooltip;
    ExmInstallButtonState state;

    tooltip = _("This extension is incompatible with your current version of GNOME.");
    state = button->state;

    gtk_widget_remove_css_class (GTK_WIDGET (button), "warning");
    gtk_widget_remove_css_class (GTK_WIDGET (button), "suggested-action");
    gtk_widget_set_tooltip_text (GTK_WIDGET (button), NULL);

    switch ((int)state)
    {
    case EXM_INSTALL_BUTTON_STATE_DEFAULT:
        gtk_button_set_label (GTK_BUTTON (button), _("Install"));
        gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
        gtk_widget_add_css_class (GTK_WIDGET (button), "suggested-action");
        break;
    case EXM_INSTALL_BUTTON_STATE_INSTALLED:
        gtk_button_set_label (GTK_BUTTON (button), C_("State", "Installed"));
        gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
        break;
    case EXM_INSTALL_BUTTON_STATE_UNSUPPORTED:
        gtk_button_set_label (GTK_BUTTON (button), _("Unsupported"));
        gtk_widget_add_css_class (GTK_WIDGET (button), "warning");
        gtk_widget_set_tooltip_text (GTK_WIDGET (button), tooltip);

        // Respect the `disable-extension-version-validation` gsetting which
        // will attempt to load extensions regardless of version incompatibility.
        gtk_widget_set_sensitive (GTK_WIDGET (button),
                                  is_version_check_disabled ());
        break;
    }
}

static void
exm_install_button_class_init (ExmInstallButtonClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_install_button_finalize;
    object_class->get_property = exm_install_button_get_property;
    object_class->set_property = exm_install_button_set_property;

    properties [PROP_STATE]
        = g_param_spec_enum ("state",
                             "State",
                             "State",
                             EXM_TYPE_INSTALL_BUTTON_STATE,
                             EXM_INSTALL_BUTTON_STATE_DEFAULT,
                             G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_install_button_init (ExmInstallButton *self)
{
}
