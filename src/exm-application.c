/* exm-application.c
 *
 * Copyright 2022 Matthew Jakeman
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
exm_application_new (gchar *application_id,
                     GApplicationFlags  flags)
{
    return g_object_new (EXM_TYPE_APPLICATION,
                       "application-id", application_id,
                       "flags", flags,
                       NULL);
}

static void
exm_application_finalize (GObject *object)
{
    ExmApplication *self = (ExmApplication *)object;

    G_OBJECT_CLASS (exm_application_parent_class)->finalize (object);
}

static ExmWindow *
get_current_window (GApplication *app)
{
    GtkWindow *window;

    /* Get the current window or create one if necessary. */
    window = gtk_application_get_active_window (GTK_APPLICATION (app));
    if (window == NULL)
    window = g_object_new (EXM_TYPE_WINDOW,
                           "application", app,
                           NULL);

    return EXM_WINDOW (window);
}

static void
exm_application_activate (GApplication *app)
{
    GtkWindow *window;
    GdkDisplay *display;
    GtkCssProvider *provider;
    GtkIconTheme *icon_theme;

    /* It's good practice to check your parameters at the beginning of the
    * function. It helps catch errors early and in development instead of
    * by your users.
    */
    g_assert (GTK_IS_APPLICATION (app));

    display = gdk_display_get_default ();
    provider = gtk_css_provider_new ();
    gtk_css_provider_load_from_resource (provider, "/com/mattjakeman/ExtensionManager/style.css");
    gtk_style_context_add_provider_for_display (display, GTK_STYLE_PROVIDER (provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    icon_theme = gtk_icon_theme_get_for_display (display);
    gtk_icon_theme_add_resource_path (icon_theme, "/com/mattjakeman/ExtensionManager/icons");

    window = GTK_WINDOW (get_current_window (app));

    /* Ask the window manager/compositor to present the window. */
    gtk_window_present (window);
}

static void
exm_application_open (GApplication  *app,
                      GFile        **files,
                      gint           n_files,
                      const gchar   *hint)
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
exm_application_class_init (ExmApplicationClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

    object_class->finalize = exm_application_finalize;

    /*
    * We connect to the activate callback to create a window when the application
    * has been launched. Additionally, this callback notifies us when the user
    * tries to launch a "second instance" of the application. When they try
    * to do that, we'll just present any existing window.
    */
    app_class->activate = exm_application_activate;
    app_class->open = exm_application_open;
}

static void
exm_application_show_about (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data)
{
    ExmApplication *self = EXM_APPLICATION (user_data);
    GtkWindow *window = NULL;
    const gchar *authors[] = {"Matthew Jakeman", NULL};

    GtkWidget *about_window;

    g_return_if_fail (EXM_IS_APPLICATION (self));

    window = gtk_application_get_active_window (GTK_APPLICATION (self));

    about_window = adw_about_window_new_from_appdata ("/com/mattjakeman/ExtensionManager/com.mattjakeman.ExtensionManager.metainfo.xml", APP_VERSION);
    gtk_window_set_modal (GTK_WINDOW (about_window), TRUE);
    gtk_window_set_transient_for (GTK_WINDOW (about_window), window);

    adw_about_window_set_comments (ADW_ABOUT_WINDOW (about_window), _("Browse, install, and manage GNOME Shell Extensions."));
    adw_about_window_set_developers (ADW_ABOUT_WINDOW (about_window), authors);
    adw_about_window_set_translator_credits (ADW_ABOUT_WINDOW (about_window), _("translator-credits"));
    adw_about_window_set_copyright (ADW_ABOUT_WINDOW (about_window), "© 2022 Matthew Jakeman");

    // Dependency Attribution
    adw_about_window_add_legal_section (ADW_ABOUT_WINDOW (about_window),
                                        "text-engine",
                                        "Copyright (C) 2022 Matthew Jakeman",
                                        GTK_LICENSE_MPL_2_0,
                                        NULL);

#if WITH_BACKTRACE
    adw_about_window_add_legal_section (ADW_ABOUT_WINDOW (about_window),
                                        "libbacktrace",
                                        "Copyright (C) 2012-2016 Free Software Foundation, Inc.",
                                        GTK_LICENSE_BSD_3,
                                        NULL);
#endif

    adw_about_window_add_legal_section (ADW_ABOUT_WINDOW (about_window),
                                        "blueprint",
                                        "Copyright (C) 2021 James Westman",
                                        GTK_LICENSE_LGPL_3_0,
                                        NULL);

    gtk_window_present (GTK_WINDOW (about_window));
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
