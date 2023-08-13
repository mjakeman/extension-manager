/* exm-error-dialog.c
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

#include "exm-error-dialog.h"

#include "exm-config.h"

#include <glib/gi18n.h>

struct _ExmErrorDialog
{
    AdwWindow parent_instance;

    char *error_string;
    GtkTextView *text_view;
    GtkLabel *instructions;
    GtkButton *new_issue_button;
};

G_DEFINE_FINAL_TYPE (ExmErrorDialog, exm_error_dialog, ADW_TYPE_WINDOW)

enum {
    PROP_0,
    PROP_ERROR_STRING,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmErrorDialog *
exm_error_dialog_new (const char *text)
{
    return g_object_new (EXM_TYPE_ERROR_DIALOG,
                         "error-string", text,
                         NULL);
}

static void
exm_error_dialog_finalize (GObject *object)
{
    ExmErrorDialog *self = (ExmErrorDialog *)object;

    G_OBJECT_CLASS (exm_error_dialog_parent_class)->finalize (object);
}

static void
exm_error_dialog_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
    ExmErrorDialog *self = EXM_ERROR_DIALOG (object);

    switch (prop_id)
    {
    case PROP_ERROR_STRING:
        g_value_set_string (value, self->error_string);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_error_dialog_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
    ExmErrorDialog *self = EXM_ERROR_DIALOG (object);

    switch (prop_id)
    {
    case PROP_ERROR_STRING:
        GtkTextBuffer *buffer;

        buffer = gtk_text_view_get_buffer (self->text_view);

        if (self->error_string)
            g_free (self->error_string);

        self->error_string = g_strdup (g_value_get_string (value));

        GString *string_builder = g_string_new ("Support Log\n");
        g_string_append_printf (string_builder, "----\n");
        g_string_append_printf (string_builder, "Version: %s\n", APP_VERSION);
        g_string_append_printf (string_builder, "Development: %s\n", IS_DEVEL ? "Yes" : "No");
        g_string_append_printf (string_builder, "Package Format: %s\n", PKG_NAME);
        g_string_append_printf (string_builder, "Status: %s\n", IS_OFFICIAL ? "Official" : "Third Party");
        g_string_append_printf (string_builder, "----\n");
        g_string_append_printf (string_builder, "%s", self->error_string);
        gtk_text_buffer_set_text (buffer, g_string_free (string_builder, FALSE), -1);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
on_copy_button_clicked (GtkButton      *button,
                        ExmErrorDialog *window)
{
    GdkDisplay *display;
    GdkClipboard *clipboard;

    // Add to clipboard
    display = gdk_display_get_default ();
    clipboard = gdk_display_get_clipboard (display);

    gdk_clipboard_set_text (clipboard, window->error_string);

    // Success indicator
    gtk_button_set_label (button, _("Copied"));
    gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
}

static void
on_new_issue_button_clicked (ExmErrorDialog *window)
{
    GtkUriLauncher *uri = gtk_uri_launcher_new ("https://github.com/mjakeman/extension-manager/issues");

    gtk_uri_launcher_launch (uri, GTK_WINDOW (window), NULL, NULL, NULL);
}

static void
exm_error_dialog_class_init (ExmErrorDialogClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_error_dialog_finalize;
    object_class->get_property = exm_error_dialog_get_property;
    object_class->set_property = exm_error_dialog_set_property;

    properties [PROP_ERROR_STRING] =
        g_param_spec_string ("error-string",
                             "Error String",
                             "Error String",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-error-dialog.ui");

    gtk_widget_class_bind_template_child (widget_class, ExmErrorDialog, text_view);
    gtk_widget_class_bind_template_child (widget_class, ExmErrorDialog, instructions);
    gtk_widget_class_bind_template_child (widget_class, ExmErrorDialog, new_issue_button);

    gtk_widget_class_bind_template_callback (widget_class, on_copy_button_clicked);
    gtk_widget_class_bind_template_callback (widget_class, on_new_issue_button_clicked);
}

static void
exm_error_dialog_init (ExmErrorDialog *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    gtk_label_set_use_markup (self->instructions, TRUE);

#if IS_OFFICIAL
    gtk_label_set_text (self->instructions, _("Please open a new issue and attach the following information:"));
    gtk_widget_set_visible (GTK_WIDGET (self->new_issue_button), TRUE);
#else
    // Translators: '%s' = Name of Distributor (e.g. "Packager123")
    char *text = g_markup_printf_escaped (_("You are using a third-party build of Extension Manager. Please <span weight=\"bold\">contact the package distributor (%s) first</span> before filing an issue. Be sure to attach the following information:"), PKG_DISTRIBUTOR);
    gtk_label_set_markup (self->instructions, text);
    gtk_widget_set_visible (GTK_WIDGET (self->new_issue_button), FALSE);
    g_free (text);
#endif
}

