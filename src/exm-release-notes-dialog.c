/* exm-release-notes-dialog.c
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

#include "exm-release-notes-dialog.h"

struct _ExmReleaseNotesDialog
{
    GtkWindow parent_instance;

    GtkTextView *text_view;
};

G_DEFINE_FINAL_TYPE (ExmReleaseNotesDialog, exm_release_notes_dialog, GTK_TYPE_WINDOW)

enum {
    PROP_0,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmReleaseNotesDialog *
exm_release_notes_dialog_new (void)
{
    return g_object_new (EXM_TYPE_RELEASE_NOTES_DIALOG, NULL);
}

static void
exm_release_notes_dialog_finalize (GObject *object)
{
    ExmReleaseNotesDialog *self = (ExmReleaseNotesDialog *)object;

    G_OBJECT_CLASS (exm_release_notes_dialog_parent_class)->finalize (object);
}

static void
exm_release_notes_dialog_class_init (ExmReleaseNotesDialogClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_release_notes_dialog_finalize;

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-release-notes-dialog.ui");
    gtk_widget_class_bind_template_child (widget_class, ExmReleaseNotesDialog, text_view);
}

static void
exm_release_notes_dialog_init (ExmReleaseNotesDialog *self)
{
    GtkTextBuffer *buffer;
    GError *error = NULL;

    GFile *file;
    gchar *contents;
    gsize length;

    gtk_widget_init_template (GTK_WIDGET (self));

    file = g_file_new_for_uri ("resource:///com/mattjakeman/ExtensionManager/release-notes.txt");

    if (!file)
    {
        g_critical ("Could not read release-notes.txt: invalid file");
        return;
    }

    if (g_file_load_contents (file, NULL, &contents, &length, NULL, &error))
    {
        buffer = gtk_text_view_get_buffer (self->text_view);
        gtk_text_buffer_set_text (GTK_TEXT_BUFFER (buffer), contents, length);
    }
    else
    {
        g_critical ("Could not read release-notes.txt: %s", error->message);
    }
}
