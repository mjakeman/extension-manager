/*
 * exm-gnome-shell-search-provider.c
 *
 * Copyright 2022-2026 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#include "exm-gnome-shell-search-provider.h"

#include <gtk/gtk.h>

#include "exm-config.h"
#include "exm-shell-search-provider-generated.h"

struct _ExmGnomeShellSearchProvider
{
    GObject parent_instance;

    ExmManager *manager;

    ExmShellSearchProvider2 *skeleton;
    GDBusConnection *connection;
    gchar *object_path;
};

G_DEFINE_FINAL_TYPE (ExmGnomeShellSearchProvider, exm_gnome_shell_search_provider, G_TYPE_OBJECT)

static gboolean
extension_matches_terms (ExmExtension       *extension,
                         const gchar *const *terms)
{
    gchar *name = NULL;
    gchar *description = NULL;
    gchar *uuid = NULL;
    gboolean matches = TRUE;

    g_object_get (extension,
                 "name", &name,
                 "description", &description,
                 "uuid", &uuid,
                 NULL);

    for (int i = 0; matches && terms[i] != NULL; i++)
    {
        g_autofree gchar *term_folded = g_utf8_casefold (terms[i], -1);
        gboolean term_matches = FALSE;

        if (name != NULL)
        {
            g_autofree gchar *name_folded = g_utf8_casefold (name, -1);
            term_matches = (strstr (name_folded, term_folded) != NULL);
        }

        if (!term_matches && description != NULL)
        {
            g_autofree gchar *description_folded = g_utf8_casefold (description, -1);
            term_matches = (strstr (description_folded, term_folded) != NULL);
        }

        if (!term_matches && uuid != NULL)
        {
            g_autofree gchar *uuid_folded = g_utf8_casefold (uuid, -1);
            term_matches = (strstr (uuid_folded, term_folded) != NULL);
        }

        matches = term_matches;
    }

    g_free (name);
    g_free (description);
    g_free (uuid);

    return matches;
}

static GVariant *
find_matching_results (ExmGnomeShellSearchProvider *self,
                       const gchar *const          *terms)
{
    GVariantBuilder builder;
    GListModel *model = NULL;
    guint n_items;

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("as"));

    g_object_get (self->manager, "extensions", &model, NULL);

    n_items = model ? g_list_model_get_n_items (model) : 0;
    for (guint i = 0; i < n_items; i++)
    {
        g_autoptr (ExmExtension) extension = g_list_model_get_item (model, i);
        gboolean has_prefs = FALSE;

        g_object_get (extension, "has-prefs", &has_prefs, NULL);

        if (has_prefs && extension_matches_terms (extension, terms))
        {
            gchar *uuid = NULL;
            g_object_get (extension, "uuid", &uuid, NULL);
            g_variant_builder_add (&builder, "s", uuid);
            g_free (uuid);
        }
    }

    g_clear_object (&model);

    return g_variant_new ("(as)", &builder);
}

static gboolean
handle_get_initial_result_set (ExmShellSearchProvider2      *skeleton G_GNUC_UNUSED,
                               GDBusMethodInvocation        *invocation,
                               gchar                        **terms,
                               ExmGnomeShellSearchProvider  *self)
{
    g_dbus_method_invocation_return_value (invocation,
                                           find_matching_results (self, (const gchar *const *) terms));
    return TRUE;
}

static gboolean
handle_get_subsearch_result_set (ExmShellSearchProvider2      *skeleton G_GNUC_UNUSED,
                                 GDBusMethodInvocation        *invocation,
                                 gchar                        **previous_results G_GNUC_UNUSED,
                                 gchar                        **terms,
                                 ExmGnomeShellSearchProvider  *self)
{
    g_dbus_method_invocation_return_value (invocation,
                                           find_matching_results (self, (const gchar *const *) terms));
    return TRUE;
}

static gboolean
handle_get_result_metas (ExmShellSearchProvider2      *skeleton G_GNUC_UNUSED,
                         GDBusMethodInvocation        *invocation,
                         gchar                        **results,
                         ExmGnomeShellSearchProvider  *self)
{
    GVariantBuilder builder;

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("aa{sv}"));

    for (int i = 0; results[i] != NULL; i++)
    {
        ExmExtension *extension = exm_manager_get_by_uuid (self->manager, results[i]);
        GVariantBuilder meta_builder;
        gchar *name = NULL;
        gchar *description = NULL;
        g_autoptr (GIcon) icon = NULL;
        GVariant *icon_variant = NULL;

        if (extension == NULL)
            continue;

        g_object_get (extension, "name", &name, "description", &description, NULL);

        g_variant_builder_init (&meta_builder, G_VARIANT_TYPE ("a{sv}"));
        g_variant_builder_add (&meta_builder, "{sv}", "id", g_variant_new_string (results[i]));
        g_variant_builder_add (&meta_builder, "{sv}", "name", g_variant_new_string (name ? name : results[i]));

        if (description != NULL && *description != '\0')
            g_variant_builder_add (&meta_builder, "{sv}", "description", g_variant_new_string (description));

        icon = g_themed_icon_new (APP_ID);
        icon_variant = g_icon_serialize (icon);
        if (icon_variant != NULL)
            g_variant_builder_add (&meta_builder, "{sv}", "icon", icon_variant);

        g_variant_builder_add_value (&builder, g_variant_builder_end (&meta_builder));

        g_free (name);
        g_free (description);
        g_object_unref (extension);
    }

    g_dbus_method_invocation_return_value (invocation, g_variant_new ("(aa{sv})", &builder));
    return TRUE;
}

static gboolean
release_application_hold (gpointer application)
{
    g_application_release (G_APPLICATION (application));
    return G_SOURCE_REMOVE;
}

static gboolean
handle_activate_result (ExmShellSearchProvider2      *skeleton,
                        GDBusMethodInvocation        *invocation,
                        gchar                        *result,
                        gchar                        **terms G_GNUC_UNUSED,
                        guint32                        timestamp G_GNUC_UNUSED,
                        ExmGnomeShellSearchProvider  *self)
{
    ExmExtension *extension;

    extension = exm_manager_get_by_uuid (self->manager, result);

    if (extension != NULL)
    {
        GApplication *application = g_application_get_default ();
        g_application_hold (application);
        g_timeout_add_seconds (2, release_application_hold, application);

        exm_manager_open_prefs (self->manager, extension);
        g_object_unref (extension);
    }

    exm_shell_search_provider2_complete_activate_result (skeleton, invocation);
    return TRUE;
}

static gboolean
handle_launch_search (ExmShellSearchProvider2      *skeleton,
                      GDBusMethodInvocation        *invocation,
                      gchar                        **terms,
                      guint32                        timestamp G_GNUC_UNUSED,
                      ExmGnomeShellSearchProvider  *self G_GNUC_UNUSED)
{
    GApplication *application;
    GtkWindow *window;

    application = g_application_get_default ();
    g_application_activate (application);

    window = gtk_application_get_active_window (GTK_APPLICATION (application));
    if (window != NULL)
    {
        g_autofree gchar *search_text = g_strjoinv (" ", terms);
        gtk_widget_activate_action (GTK_WIDGET (window), "win.search-installed", "s", search_text);
    }

    exm_shell_search_provider2_complete_launch_search (skeleton, invocation);
    return TRUE;
}

ExmGnomeShellSearchProvider *
exm_gnome_shell_search_provider_new (ExmManager *manager)
{
    ExmGnomeShellSearchProvider *self;

    self = g_object_new (EXM_TYPE_GNOME_SHELL_SEARCH_PROVIDER, NULL);
    self->manager = g_object_ref (manager);

    return self;
}

gboolean
exm_gnome_shell_search_provider_export (ExmGnomeShellSearchProvider  *self,
                                        GDBusConnection              *connection,
                                        const gchar                  *object_path,
                                        GError                      **error)
{
    g_return_val_if_fail (EXM_IS_GNOME_SHELL_SEARCH_PROVIDER (self), FALSE);

    if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self->skeleton),
                                           connection, object_path, error))
        return FALSE;

    self->connection = g_object_ref (connection);
    self->object_path = g_strdup (object_path);

    return TRUE;
}

void
exm_gnome_shell_search_provider_unexport (ExmGnomeShellSearchProvider *self)
{
    g_return_if_fail (EXM_IS_GNOME_SHELL_SEARCH_PROVIDER (self));

    if (self->connection == NULL)
        return;

    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (self->skeleton));

    g_clear_object (&self->connection);
    g_clear_pointer (&self->object_path, g_free);
}

static void
exm_gnome_shell_search_provider_dispose (GObject *object)
{
    ExmGnomeShellSearchProvider *self = EXM_GNOME_SHELL_SEARCH_PROVIDER (object);

    exm_gnome_shell_search_provider_unexport (self);

    g_clear_object (&self->manager);
    g_clear_object (&self->skeleton);

    G_OBJECT_CLASS (exm_gnome_shell_search_provider_parent_class)->dispose (object);
}

static void
exm_gnome_shell_search_provider_class_init (ExmGnomeShellSearchProviderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = exm_gnome_shell_search_provider_dispose;
}

static void
exm_gnome_shell_search_provider_init (ExmGnomeShellSearchProvider *self)
{
    self->skeleton = exm_shell_search_provider2_skeleton_new ();

    g_signal_connect (self->skeleton, "handle-get-initial-result-set",
                      G_CALLBACK (handle_get_initial_result_set), self);
    g_signal_connect (self->skeleton, "handle-get-subsearch-result-set",
                      G_CALLBACK (handle_get_subsearch_result_set), self);
    g_signal_connect (self->skeleton, "handle-get-result-metas",
                      G_CALLBACK (handle_get_result_metas), self);
    g_signal_connect (self->skeleton, "handle-activate-result",
                      G_CALLBACK (handle_activate_result), self);
    g_signal_connect (self->skeleton, "handle-launch-search",
                      G_CALLBACK (handle_launch_search), self);
}
