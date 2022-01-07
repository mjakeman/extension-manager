/* exm-window.c
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
#include "exm-window.h"

#include "model/exm-manager.h"
#include "model/exm-extension.h"

#include <adwaita.h>

struct _ExmWindow
{
    GtkApplicationWindow  parent_instance;

    /* Template widgets */
    AdwHeaderBar        *header_bar;
    GtkListBox          *list_box;
};

G_DEFINE_TYPE (ExmWindow, exm_window, GTK_TYPE_APPLICATION_WINDOW)

static void
exm_window_class_init (ExmWindowClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-window.ui");
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, header_bar);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, list_box);
}

static GtkWidget *
widget_factory (ExmExtension* extension)
{
    GtkWidget *row;
    GtkWidget *label;

    char *name, *uuid, *description;
    g_object_get (extension,
                  "display-name", &name,
                  "uuid", &uuid,
                  "description", &description,
                  NULL);

    name = g_markup_escape_text (name, -1);

    row = adw_expander_row_new ();

    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), name);
    adw_expander_row_set_subtitle (ADW_EXPANDER_ROW (row), uuid);

    label = gtk_label_new (description);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_label_set_wrap (GTK_LABEL (label), GTK_WRAP_WORD);
    gtk_widget_add_css_class (label, "description-label");
    adw_expander_row_add_row (ADW_EXPANDER_ROW (row), label);

    return row;
}

static void
exm_window_init (ExmWindow *self)
{
    ExmManager *manager;
    GListModel *model;

    gtk_widget_init_template (GTK_WIDGET (self));

    manager = exm_manager_new ();
    g_object_get (manager, "list-model", &model, NULL);

    gtk_list_box_bind_model (self->list_box, model,
                             (GtkListBoxCreateWidgetFunc)widget_factory,
                             NULL, NULL);
}
