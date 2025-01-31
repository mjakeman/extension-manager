/*
 * exm-application.c
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

#include "exm-config.h"

#include "exm-application.h"
#include "exm-window.h"

#include "exm-utils.h"

#include <glib/gi18n.h>

struct _ExmApplication
{
    AdwApplication parent_instance;
};

G_DEFINE_TYPE (ExmApplication, exm_application, ADW_TYPE_APPLICATION)

ExmApplication *
exm_application_new (gchar             *application_id,
                     GApplicationFlags  flags)
{
    return g_object_new (EXM_TYPE_APPLICATION,
                         "application-id", application_id,
                         "flags", flags,
                         NULL);
}

static void
save_window_state (GtkWindow *self,
                   gpointer   user_data G_GNUC_UNUSED)
{
    GSettings *settings;
    gint default_width, default_height;
    gboolean maximized;

    settings = g_settings_new (APP_ID);

    g_object_get (self, "default-width", &default_width, NULL);
    g_object_get (self, "default-height", &default_height, NULL);
    g_object_get (self, "maximized", &maximized, NULL);

    g_settings_set_int (settings, "width", default_width);
    g_settings_set_int (settings, "height", default_height);
    g_settings_set_boolean (settings, "is-maximized", maximized);

    g_object_unref (settings);
}

static ExmWindow *
get_current_window (GApplication *app)
{
    GSettings *settings;
    GtkWindow *window;

    settings = g_settings_new (APP_ID);

    /* Get the current window or create one if necessary. */
    window = gtk_application_get_active_window (GTK_APPLICATION (app));
    if (window == NULL)
    {
        window = g_object_new (EXM_TYPE_WINDOW,
                               "application", app,
                               "default-width", g_settings_get_int (settings, "width"),
                               "default-height", g_settings_get_int (settings, "height"),
                               "maximized", g_settings_get_boolean (settings, "is-maximized"),
                               NULL);

        g_signal_connect (window, "close-request", G_CALLBACK (save_window_state), NULL);
    }

    g_object_unref (settings);

    return EXM_WINDOW (window);
}

static void
exm_application_activate (GApplication *app)
{
    GtkWindow *window;

    /* It's good practice to check your parameters at the beginning of the
    * function. It helps catch errors early and in development instead of
    * by your users.
    */
    g_assert (GTK_IS_APPLICATION (app));

    window = GTK_WINDOW (get_current_window (app));

    /* Ask the window manager/compositor to present the window. */
    gtk_window_present (window);
}

static void
exm_application_open (GApplication  *app,
                      GFile        **files,
                      gint           n_files,
                      const gchar   *hint G_GNUC_UNUSED)
{
    ExmWindow *window;
    const char *scheme;
    const char *uuid;
    GUri *uri;
    GError *error = NULL;

    // Activate the application first
    exm_application_activate (app);

    // Now open the provided extension
    window = get_current_window (app);

    if (n_files <= 0)
        return;

    uri = g_uri_parse (g_file_get_uri (files[0]), G_URI_FLAGS_NONE, &error);
    if (error)
    {
        g_critical ("Error parsing URI: %s\n", error->message);
        return;
    }

    scheme = g_uri_get_scheme (uri);
    if (!g_str_equal (scheme, "gnome-extensions"))
    {
        g_critical ("Invalid URI scheme: '%s'\n", scheme);
        return;
    }

    uuid = g_uri_get_host (uri);
    g_info ("Opening extension with UUID: '%s'\n", uuid);
    gtk_widget_activate_action (GTK_WIDGET (window), "win.show-detail", "s", uuid);
}

static void
exm_application_shutdown (GApplication *app)
{
    save_window_state (GTK_WINDOW (get_current_window (app)), NULL);

    G_APPLICATION_CLASS (exm_application_parent_class)->shutdown (app);
}

static void
exm_application_class_init (ExmApplicationClass *klass)
{
    GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

    /*
    * We connect to the activate callback to create a window when the application
    * has been launched. Additionally, this callback notifies us when the user
    * tries to launch a "second instance" of the application. When they try
    * to do that, we'll just present any existing window.
    */
    app_class->activate = exm_application_activate;
    app_class->open = exm_application_open;
    app_class->shutdown = exm_application_shutdown;
}

static void
exm_application_show_about (GSimpleAction *action G_GNUC_UNUSED,
                            GVariant      *parameter G_GNUC_UNUSED,
                            gpointer       user_data)
{
    ExmApplication *self = EXM_APPLICATION (user_data);
    GtkWindow *window = NULL;
    const gchar *authors[] = {"Matthew Jakeman", NULL};

    AdwDialog *about_dialog;

    g_return_if_fail (EXM_IS_APPLICATION (self));

    window = gtk_application_get_active_window (GTK_APPLICATION (self));

    about_dialog = adw_about_dialog_new_from_appdata (g_strdup_printf ("%s/com.mattjakeman.ExtensionManager.metainfo.xml", RESOURCE_PATH),
                                                      strstr (APP_ID, ".Devel") == NULL ? APP_VERSION : NULL);

    adw_about_dialog_set_version (ADW_ABOUT_DIALOG (about_dialog), APP_VERSION);
    adw_about_dialog_set_comments (ADW_ABOUT_DIALOG (about_dialog), _("Browse, install, and manage GNOME Shell Extensions."));
    adw_about_dialog_set_developers (ADW_ABOUT_DIALOG (about_dialog), authors);
    adw_about_dialog_set_translator_credits (ADW_ABOUT_DIALOG (about_dialog), _("translator-credits"));
    adw_about_dialog_set_copyright (ADW_ABOUT_DIALOG (about_dialog), "© 2022-2025 Matthew Jakeman");

#if WITH_BACKTRACE
    adw_about_dialog_add_legal_section (ADW_ABOUT_DIALOG (about_dialog),
                                        "libbacktrace",
                                        "© 2012-2016 Free Software Foundation, Inc.",
                                        GTK_LICENSE_BSD_3,
                                        NULL);
#endif

    adw_about_dialog_add_legal_section (ADW_ABOUT_DIALOG (about_dialog),
                                        "blueprint",
                                        "© 2021 James Westman",
                                        GTK_LICENSE_LGPL_3_0,
                                        NULL);

    adw_dialog_present (about_dialog, GTK_WIDGET (window));
}

static void
request_logout (ExmApplication *self)
{
    // Request the GNOME Session Manager to log out
    GDBusConnection *conn;

    conn = g_application_get_dbus_connection (G_APPLICATION (self));
    if (conn)
    {
        GError *error = NULL;
        g_dbus_connection_call_sync (conn,
                                     "org.gnome.SessionManager",
                                     "/org/gnome/SessionManager",
                                     "org.gnome.SessionManager",
                                     "Logout",
                                     g_variant_new ("(u)", 0),
                                     NULL,
                                     G_DBUS_CALL_FLAGS_NONE,
                                     -1,
                                     NULL,
                                     &error);

        if (error)
            g_warning ("Could not log out: %s", error->message);
    }
}

static void
exm_application_init (ExmApplication *self)
{
    GSettings *settings = g_settings_new (APP_ID);

    GSimpleAction *quit_action = g_simple_action_new ("quit", NULL);
    g_signal_connect_swapped (quit_action, "activate", G_CALLBACK (g_application_quit), self);
    g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (quit_action));

    GSimpleAction *about_action = g_simple_action_new ("about", NULL);
    g_signal_connect (about_action, "activate", G_CALLBACK (exm_application_show_about), self);
    g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (about_action));

    GSimpleAction *logout_action = g_simple_action_new ("logout", NULL);
    g_signal_connect_swapped (logout_action, "activate", G_CALLBACK (request_logout), self);
    g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (logout_action));

    GAction *sort_enabled_first_action = g_settings_create_action (settings, "sort-enabled-first");
    g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (sort_enabled_first_action));

    GAction *show_unsupported_action = g_settings_create_action (settings, "show-unsupported");
    g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (show_unsupported_action));

    g_object_unref (settings);

    const char *accels_quit[] = {"<primary>q", NULL};
    gtk_application_set_accels_for_action (GTK_APPLICATION (self), "app.quit", accels_quit);

    const char *accels_close[] = {"<primary>w", NULL};
    gtk_application_set_accels_for_action (GTK_APPLICATION (self), "window.close", accels_close);
}
