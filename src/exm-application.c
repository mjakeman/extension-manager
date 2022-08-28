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

static void
exm_application_activate (GApplication *app)
{
    GtkWindow *window;

    /* It's good practice to check your parameters at the beginning of the
    * function. It helps catch errors early and in development instead of
    * by your users.
    */
    g_assert (GTK_IS_APPLICATION (app));

    GdkDisplay *display = gdk_display_get_default ();
    GtkCssProvider *provider = gtk_css_provider_new ();
    gtk_css_provider_load_from_resource (provider, "/com/mattjakeman/ExtensionManager/style.css");
    gtk_style_context_add_provider_for_display (display, GTK_STYLE_PROVIDER (provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    /* Get the current window or create one if necessary. */
    window = gtk_application_get_active_window (GTK_APPLICATION (app));
    if (window == NULL)
    window = g_object_new (EXM_TYPE_WINDOW,
                           "application", app,
                           NULL);

    /* Ask the window manager/compositor to present the window. */
    gtk_window_present (window);
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
}

static void
exm_application_show_about (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data)
{
    ExmApplication *self = EXM_APPLICATION (user_data);
    GtkWindow *window = NULL;
    const gchar *authors[] = {"Matthew Jakeman", NULL};
    const gchar *program_name = IS_DEVEL
        ? _("Extension Manager (Development)")
        : _("Extension Manager");

    g_return_if_fail (EXM_IS_APPLICATION (self));

    window = gtk_application_get_active_window (GTK_APPLICATION (self));

    adw_show_about_window (window,
                           "application-name", program_name,
                           "application-icon", APP_ID,
                           "developer-name", "Matthew Jakeman",
                           "version", APP_VERSION,
                           "comments", _("A very simple tool for browsing, downloading, and managing GNOME shell extensions."),
                           "website", "https://github.com/mjakeman/extension-manager",
                           "support-url", "https://github.com/mjakeman/extension-manager/discussions",
                           "issue-url", "https://github.com/mjakeman/extension-manager/issues",
                           // "debug-info", "<system stats>",
                           "developers", authors,
                           "translator-credits", _("translator-credits"),
                           "copyright", "© 2022 Matthew Jakeman",
                           "license-type", GTK_LICENSE_GPL_3_0,
                           NULL);
}


static gboolean
map_setting_to_adw_style (GValue   *value,
                          GVariant *variant)
{
    char *str;

    g_variant_get (variant, "s", &str);

    if (strcmp (str, "use-default") == 0)
    {
        g_value_set_enum (value, ADW_COLOR_SCHEME_DEFAULT);
        return TRUE;
    }
    else if (strcmp (str, "force-light") == 0)
    {
        g_value_set_enum (value, ADW_COLOR_SCHEME_FORCE_LIGHT);
        return TRUE;
    }
    else if (strcmp (str, "force-dark") == 0)
    {
        g_value_set_enum (value, ADW_COLOR_SCHEME_FORCE_DARK);
        return TRUE;
    }

    return FALSE; // error
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

    GAction *style_variant_action = g_settings_create_action (settings, "style-variant");
    g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (style_variant_action));

    AdwStyleManager *style_manager = adw_style_manager_get_default ();
    g_settings_bind_with_mapping (settings, "style-variant",
                                  style_manager, "color-scheme",
                                  G_SETTINGS_BIND_GET,
                                  map_setting_to_adw_style,
                                  NULL, NULL, NULL);

    g_object_unref (settings);

    const char *accels[] = {"<primary>q", NULL};
    gtk_application_set_accels_for_action (GTK_APPLICATION (self), "app.quit", accels);
}
