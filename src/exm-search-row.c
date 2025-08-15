/*
 * exm-search-row.c
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

#include "exm-search-row.h"

#include "exm-config.h"
#include "exm-enums.h"
#include "exm-install-button.h"
#include "exm-types.h"
#include "web/exm-versions-provider.h"
#include "web/model/exm-version-result.h"

#include <glib/gi18n.h>

struct _ExmSearchRow
{
    GtkListBoxRow parent_instance;

    ExmManager *manager;
    ExmSearchResult *search_result;
    ExmVersionsProvider *versions_provider;
    gboolean compact;
    gchar *uuid;

    GtkLabel *description_label;
    ExmInstallButton *install_btn;
    guint signal_id;
    gboolean install_attempt;
};

G_DEFINE_FINAL_TYPE (ExmSearchRow, exm_search_row, GTK_TYPE_LIST_BOX_ROW)

enum {
    PROP_0,
    PROP_MANAGER,
    PROP_SEARCH_RESULT,
    PROP_COMPACT,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmSearchRow *
exm_search_row_new (ExmManager      *manager,
                    ExmSearchResult *search_result)
{
    return g_object_new (EXM_TYPE_SEARCH_ROW,
                         "manager", manager,
                         "search-result", search_result,
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
    case PROP_MANAGER:
        g_value_set_object (value, self->manager);
        break;
    case PROP_SEARCH_RESULT:
        g_value_set_object (value, self->search_result);
        break;
    case PROP_COMPACT:
        g_value_set_boolean (value, self->compact);
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
    case PROP_MANAGER:
        self->manager = g_value_get_object (value);
        break;
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
    case PROP_COMPACT:
        self->compact = g_value_get_boolean (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
install_extension (ExmInstallButtonState  install_state,
                   ExmSearchRow          *self)
{
    gboolean warn;

    warn = (install_state == EXM_INSTALL_BUTTON_STATE_UNSUPPORTED);

    g_object_set (self->install_btn, "state", EXM_INSTALL_BUTTON_STATE_INSTALLING, NULL);

    gtk_widget_grab_focus (GTK_WIDGET (self));

    gtk_widget_activate_action (GTK_WIDGET (self->install_btn),
                                "ext.install",
                                "(sb)", self->uuid, warn);
}

static void
on_version_loaded (GObject      *source,
                   GAsyncResult *result,
                   gpointer      user_data)
{
    gchar *next;
    GError *error = NULL;
    GListModel *list;
    ExmSearchRow *self;
    gboolean is_installed;
    ExmInstallButtonState install_state;
    ExmVersionResult *compatible_version = NULL;

    list = exm_versions_provider_query_finish (EXM_VERSIONS_PROVIDER (source), result, &next, &error);
    self = EXM_SEARCH_ROW (user_data);

    is_installed = exm_manager_is_installed_uuid (self->manager, self->uuid);

    if (error)
    {
        install_state = is_installed ? EXM_INSTALL_BUTTON_STATE_INSTALLED
                                     : EXM_INSTALL_BUTTON_STATE_UNSUPPORTED;

        g_object_set (self->install_btn, "state", install_state, NULL);

        g_clear_error (&error);
        return;
    }

    const gchar *shell_version;
    g_object_get (self->versions_provider, "shell-version", &shell_version, NULL);

    for (guint i = 0; i < g_list_model_get_n_items (list); i++)
    {
        ExmVersionResult *item = EXM_VERSION_RESULT (g_list_model_get_object (list, i));

        if (exm_version_result_supports_shell_version (item, shell_version))
        {
            if (compatible_version)
                g_object_unref (compatible_version);
            compatible_version = item;
        }
        else
        {
            g_object_unref (item);
        }
    }

    if (compatible_version)
    {
        install_state = is_installed ? EXM_INSTALL_BUTTON_STATE_INSTALLED
                                     : EXM_INSTALL_BUTTON_STATE_DEFAULT;

        g_object_set (self->install_btn, "state", install_state, NULL);

        g_object_unref (compatible_version);
    }
    else if (next != NULL)
    {
        exm_versions_provider_query_next_async (self->versions_provider,
                                                next,
                                                NULL,
                                                on_version_loaded,
                                                self);
        g_object_unref (list);
        return;
    }
    else
    {
        install_state = is_installed ? EXM_INSTALL_BUTTON_STATE_INSTALLED
                                     : EXM_INSTALL_BUTTON_STATE_UNSUPPORTED;

        g_object_set (self->install_btn, "state", install_state, NULL);
    }

    g_object_unref (list);

    install_extension (install_state, self);
}

static void
on_install_status (ExmManager            *manager G_GNUC_UNUSED,
                   ExmInstallButtonState  state,
                   ExmSearchRow          *self)
{
    g_object_set (self->install_btn, "state", state, NULL);

    if (self->signal_id > 0)
        g_signal_handler_disconnect (self->manager, self->signal_id);
}

static void
install_remote (GtkButton    *button G_GNUC_UNUSED,
                ExmSearchRow *self)
{
    self->signal_id = g_signal_connect_object (self->manager,
                                               "install-status",
                                               G_CALLBACK (on_install_status),
                                               self,
                                               G_CONNECT_DEFAULT);

    if (!self->install_attempt)
    {
        self->install_attempt = TRUE;

        g_object_set (self->install_btn, "state", EXM_INSTALL_BUTTON_STATE_LOADING, NULL);

        exm_versions_provider_query_async (self->versions_provider, self->uuid, NULL, on_version_loaded, self);
    }
    else
    {
        ExmInstallButtonState state;

        g_object_get (self->install_btn, "state", &state, NULL);

        install_extension (state, self);
    }
}

static void
exm_search_row_constructed (GObject *object)
{
    ExmSearchRow *self = EXM_SEARCH_ROW (object);

    ExmInstallButtonState install_state;
    gboolean is_installed;

    gchar *uuid, *description;
    g_object_get (self->search_result,
                  "uuid", &uuid,
                  "description", &description,
                  NULL);

    gchar *shell_version;
    g_object_get (self->manager,
                  "shell-version",
                  &shell_version,
                  NULL);

    g_object_set (self->versions_provider, "shell-version", shell_version, NULL);

    gtk_actionable_set_action_target (GTK_ACTIONABLE (self), "s", uuid);
    is_installed = exm_manager_is_installed_uuid (self->manager, uuid);

    install_state = is_installed
        ? EXM_INSTALL_BUTTON_STATE_INSTALLED
        : EXM_INSTALL_BUTTON_STATE_DEFAULT;

    g_object_set (self->install_btn, "state", install_state, NULL);

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

    properties [PROP_MANAGER]
        = g_param_spec_object ("manager",
                               "Manager",
                               "Manager",
                               EXM_TYPE_MANAGER,
                               G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_SEARCH_RESULT] =
        g_param_spec_object ("search-result",
                             "Search Result",
                             "Search Result",
                             EXM_TYPE_SEARCH_RESULT,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_COMPACT] =
        g_param_spec_boolean ("compact",
                              "Compact",
                              "Compact",
                              FALSE,
                              G_PARAM_READWRITE);

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

    self->versions_provider = exm_versions_provider_new ();
    self->install_attempt = FALSE;
}
