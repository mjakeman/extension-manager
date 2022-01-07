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
    GtkListView         *list_view;
};

G_DEFINE_TYPE (ExmWindow, exm_window, GTK_TYPE_APPLICATION_WINDOW)

static void
exm_window_class_init (ExmWindowClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-window.ui");
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, header_bar);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, list_view);
}

static void
setup_listitem_cb (GtkListItemFactory *factory,
                   GtkListItem        *list_item)
{
    GtkWidget *label = gtk_label_new ("");
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_list_item_set_child (list_item, label);
}

static void
bind_listitem_cb (GtkListItemFactory *factory,
                  GtkListItem        *list_item)
{
    GtkWidget *label;
    label = gtk_list_item_get_child (list_item);
    ExmExtension *extension = gtk_list_item_get_item (list_item);

    char *name;
    g_object_get (extension,
                  "display-name", &name,
                  NULL);

    gtk_label_set_label (GTK_LABEL (label), name);
}

static void
exm_window_init (ExmWindow *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    GtkListItemFactory *ext_factory = gtk_signal_list_item_factory_new ();
    g_signal_connect (ext_factory, "setup", G_CALLBACK (setup_listitem_cb), NULL);
    g_signal_connect (ext_factory, "bind", G_CALLBACK (bind_listitem_cb), NULL);

    gtk_list_view_set_factory (self->list_view, ext_factory);

    ExmManager *manager = exm_manager_new ();
    GListModel *model;
    g_object_get (manager, "list-model", &model, NULL);
    gtk_list_view_set_model (self->list_view, GTK_SELECTION_MODEL (gtk_no_selection_new (model)));
}
