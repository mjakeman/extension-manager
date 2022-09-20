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

#include <glib/gi18n.h>

#define ISSUE_URL "https://github.com/mjakeman/extension-manager/issues"

struct _ExmErrorDialog
{
    AdwWindow parent_instance;

    char *error_string;
    GtkTextView *text_view;
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
        gtk_text_buffer_set_text (buffer, self->error_string, -1);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
on_copy_button_clicked (GtkWidget      *button,
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
    gtk_show_uri (GTK_WINDOW (window), ISSUE_URL, GDK_CURRENT_TIME);
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
	gtk_widget_class_bind_template_child (widget_class, ExmErrorDialog, new_issue_button);

    gtk_widget_class_bind_template_callback (widget_class, on_copy_button_clicked);
    gtk_widget_class_bind_template_callback (widget_class, on_new_issue_button_clicked);
}

static void
exm_error_dialog_init (ExmErrorDialog *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

	gtk_widget_set_tooltip_text (GTK_WIDGET (self->new_issue_button), ISSUE_URL);
}