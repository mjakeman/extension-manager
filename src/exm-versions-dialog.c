/*
 * exm-versions-dialog.c
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

#include "exm-versions-dialog.h"

#include "exm-config.h"

#include <glib/gi18n.h>

struct _ExmVersionsDialog
{
    AdwDialog parent_instance;

    GtkFlowBox *flow_box;
};

G_DEFINE_FINAL_TYPE (ExmVersionsDialog, exm_versions_dialog, ADW_TYPE_DIALOG)

ExmVersionsDialog *
exm_versions_dialog_new ()
{
    return g_object_new (EXM_TYPE_VERSIONS_DIALOG, NULL);
}

void
exm_versions_dialog_add_version (ExmVersionsDialog *self,
                                 gchar *version)
{
    GtkWidget *label;

    label = gtk_label_new (version);
    gtk_widget_add_css_class (label, "version-label");
    gtk_flow_box_prepend (self->flow_box, label);
}

static void
exm_versions_dialog_class_init (ExmVersionsDialogClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-versions-dialog.ui", RESOURCE_PATH));

    gtk_widget_class_bind_template_child (widget_class, ExmVersionsDialog, flow_box);
}

static void
exm_versions_dialog_init (ExmVersionsDialog *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}
