/* main.c
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

#include <glib/gi18n.h>
#include <signal.h>
#include <sys/wait.h>

#include "exm-config.h"
#include "exm-application.h"

#include "exm-backtrace.h"
#include "exm-error-dialog.h"

#define APP_URL "https://github.com/mjakeman/extension-manager"

static int pipe_fd[2];

void
handler (int sig)
{
    const char *backtrace;

    g_print ("A fatal error has occurred.\n");
    g_print ("Please report this to '%s' and attach the following crash report:\n\n", APP_URL);

    g_print ("START BACKTRACE\n\n");
    backtrace = exm_backtrace_print ();
    g_print ("%s\n", backtrace);
    g_print ("END BACKTRACE\n\n");

    if (backtrace)
    {
        // Send backtrace string over pipe
        write (pipe_fd[1], backtrace, strlen (backtrace));
    }

    close (pipe_fd[1]);

    // Terminate process
    signal (sig, SIG_DFL);
    kill (getpid (), sig);
}

static void
run_crash_reporter (const char *error_text)
{
    adw_init ();

    // Setup CSS
    GdkDisplay *display = gdk_display_get_default ();
    GtkCssProvider *provider = gtk_css_provider_new ();
    gtk_css_provider_load_from_resource (provider, g_strdup_printf ("%s/style.css", RESOURCE_PATH));
    gtk_style_context_add_provider_for_display (display, GTK_STYLE_PROVIDER (provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // Show error dialog with provided string
	ExmErrorDialog *err_dialog;
    err_dialog = exm_error_dialog_new (error_text);

    gtk_window_present (GTK_WINDOW (err_dialog));

    // Iterate main loop until closed
    while (g_list_model_get_n_items (gtk_window_get_toplevels ()) > 0)
        g_main_context_iteration (NULL, TRUE);
}

static int
run_app (int   argc,
         char *argv[])
{
	g_autoptr(ExmApplication) app = NULL;
    int ret;

    /* Setup backtrace service */
    exm_backtrace_init (argv[0]);
    signal (SIGSEGV, handler);

	/*
	 * Create a new GtkApplication. The application manages our main loop,
	 * application windows, integration with the window manager/compositor, and
	 * desktop features such as file opening and single-instance applications.
	 */
	app = exm_application_new (APP_ID, G_APPLICATION_HANDLES_OPEN);

	/*
	 * Run the application. This function will block until the application
	 * exits. Upon return, we have our exit code to return to the shell. (This
	 * is the code you see when you do `echo $?` after running a command in a
	 * terminal.
	 *
	 * Since GtkApplication inherits from GApplication, we use the parent class
	 * method "run". But we need to cast, which is what the "G_APPLICATION()"
	 * macro does.
	 */
	ret = g_application_run (G_APPLICATION (app), argc, argv);

	return ret;
}

int
main (int   argc,
      char *argv[])
{
    gboolean use_crash_reporter;
    int pid;

    // Either side of the pipe

    use_crash_reporter = TRUE;

	// Set up gettext translations
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

    // Attempt to create the pipe
    if (pipe (pipe_fd) == -1)
        use_crash_reporter = FALSE;

    // Run app normally
    if (!use_crash_reporter)
        return run_app (argc, argv);

    // Run the GUI as a subprocess of the crash reporter. Depending
    // on the exit code, we can display the crash dialog.
    pid = fork();

    // Child process
    if (pid == 0)
    {
        // Close reading end of pipe
        close (pipe_fd[0]);

        // Run app normally
        return run_app (argc, argv);
    }

    // Parent process
    else
    {
        char ch;
        GString *string_builder;
        char *error_text;

        // Close the writing end of pipe
        close (pipe_fd [1]);

        string_builder = g_string_new ("");

        while (read (pipe_fd[0], &ch, 1) != 0)
            g_string_append_c (string_builder, ch);

        // Wait for child to finish
        waitpid (pid, 0, 0);
        close (pipe_fd[0]);

        error_text = g_string_free (string_builder, FALSE);

        if (strlen (error_text) > 0)
        {
            // An error has occurred
            run_crash_reporter (error_text);
            g_free (error_text);
            return -1;
        }

        return 0;
    }
}
