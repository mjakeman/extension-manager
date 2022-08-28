/*
 * exm-upgrade-assistant.c
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
#include "exm-upgrade-assistant.h"

#include "web/exm-data-provider.h"

struct _ExmUpgradeAssistant
{
    AdwWindow parent_instance;

    ExmManager *manager;
    ExmDataProvider *data_provider;

    gchar *current_shell_version;
    gchar *target_shell_version;

    GtkListView *list_view;
    GtkDropDown *drop_down;
    GtkButton *run_button;
};

G_DEFINE_FINAL_TYPE (ExmUpgradeAssistant, exm_upgrade_assistant, ADW_TYPE_WINDOW)

enum {
    PROP_0,
    PROP_MANAGER,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmUpgradeAssistant *
exm_upgrade_assistant_new (ExmManager *manager)
{
    return g_object_new (EXM_TYPE_UPGRADE_ASSISTANT,
                         "manager", manager,
                         NULL);
}

static void
exm_upgrade_assistant_finalize (GObject *object)
{
    ExmUpgradeAssistant *self = (ExmUpgradeAssistant *)object;

    G_OBJECT_CLASS (exm_upgrade_assistant_parent_class)->finalize (object);
}

static void
exm_upgrade_assistant_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
    ExmUpgradeAssistant *self = EXM_UPGRADE_ASSISTANT (object);

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
exm_upgrade_assistant_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
    ExmUpgradeAssistant *self = EXM_UPGRADE_ASSISTANT (object);

    switch (prop_id)
    {
    case PROP_MANAGER:
        self->manager = g_value_get_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
on_data_loaded (GObject      *source,
                GAsyncResult *result,
                gpointer      user_data)
{
    ExmSearchResult *data;
    GError *error = NULL;
    ExmUpgradeAssistant *self;
    GtkWidget *child;
    GList *version_iter;
    gchar *uri;

    self = EXM_UPGRADE_ASSISTANT (user_data);

    if ((data = exm_data_provider_get_finish (EXM_DATA_PROVIDER (source), result, &error)) != FALSE)
    {
        gint pk;
        gboolean is_supported;
        gchar *uuid, *name, *creator, *icon_uri, *screenshot_uri, *link, *description;
        g_object_get (data,
                      "uuid", &uuid,
                      "name", &name,
                      "creator", &creator,
                      "icon", &icon_uri,
                      "screenshot", &screenshot_uri,
                      "link", &link,
                      "description", &description,
                      "pk", &pk,
                      NULL);

        is_supported = exm_search_result_supports_shell_version (data, self->target_shell_version);

        g_print ("Extension '%s' is supported on GNOME %s: %d\n", name, self->target_shell_version, is_supported);

    }
}

static void
do_compatibility_check (ExmUpgradeAssistant *self)
{
    GListModel *user_ext_model;
    GListModel *system_ext_model;
    int num_items;
    int i;

    if (!self->manager)
        return;

    g_object_get (self->manager,
                  "user-extensions", &user_ext_model,
                  "system-extensions", &system_ext_model,
                  NULL);

    num_items = g_list_model_get_n_items (user_ext_model);
    for (i = 0; i < num_items; i++) {
        char *uuid;
        ExmExtension *extension;

        extension = EXM_EXTENSION (g_list_model_get_item (user_ext_model, i));

        g_object_get (extension, "uuid", &uuid, NULL);
        g_print ("Processing: %s\n", uuid);

        exm_data_provider_get_async (self->data_provider, uuid, NULL, on_data_loaded, self);
    }

}

static void
exm_upgrade_assistant_class_init (ExmUpgradeAssistantClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_upgrade_assistant_finalize;
    object_class->get_property = exm_upgrade_assistant_get_property;
    object_class->set_property = exm_upgrade_assistant_set_property;

    properties [PROP_MANAGER]
        = g_param_spec_object ("manager",
                               "Manager",
                               "Manager",
                               EXM_TYPE_MANAGER,
                               G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-upgrade-assistant.ui");
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, list_view);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, run_button);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, drop_down);
}

static void
exm_upgrade_assistant_init (ExmUpgradeAssistant *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    g_signal_connect_swapped (self->run_button,
                              "clicked",
                              G_CALLBACK (do_compatibility_check),
                              self);

    self->data_provider = exm_data_provider_new ();

    self->target_shell_version = "42";
    self->current_shell_version = "41";
}

