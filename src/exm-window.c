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

#include "exm-browse-page.h"
#include "exm-installed-page.h"

#include "local/exm-manager.h"
#include "local/exm-extension.h"

#include <glib/gi18n.h>

struct _ExmWindow
{
    AdwApplicationWindow  parent_instance;

    ExmManager *manager;

    /* Template widgets */
    AdwHeaderBar        *header_bar;
    GtkSwitch           *global_toggle;
    ExmBrowsePage       *browse_page;
    ExmInstalledPage    *installed_page;
    AdwLeaflet          *leaflet;
    GtkWidget           *main_view;
    GtkWidget           *detail_view;
};

G_DEFINE_TYPE (ExmWindow, exm_window, ADW_TYPE_APPLICATION_WINDOW)

enum {
    PROP_0,
    PROP_MANAGER,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
exm_window_finalize (GObject *object)
{
    ExmWindow *self = (ExmWindow *)object;

    G_OBJECT_CLASS (exm_window_parent_class)->finalize (object);
}

static void
exm_window_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
    ExmWindow *self = EXM_WINDOW (object);

    switch (prop_id)
    {
    case PROP_MANAGER:
        g_value_set_object (value, self->manager);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_window_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
    ExmWindow *self = EXM_WINDOW (object);

    switch (prop_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
extension_open_prefs (GtkWidget  *widget,
                      const char *action_name,
                      GVariant   *param)
{
    ExmWindow *self;
    ExmExtension *extension;
    gchar *uuid;

    self = EXM_WINDOW (widget);
    g_variant_get (param, "s", &uuid);

    extension = exm_manager_get_by_uuid (self->manager, uuid);

    exm_manager_open_prefs (self->manager, extension);
}

static void
extension_state_set (GtkWidget  *widget,
                     const char *action_name,
                     GVariant   *param)
{
    ExmWindow *self;
    ExmExtension *extension;
    gchar *uuid;
    gboolean state;

    self = EXM_WINDOW (widget);
    g_variant_get (param, "(sb)", &uuid, &state);

    extension = exm_manager_get_by_uuid (self->manager, uuid);

    if (state)
        exm_manager_enable_extension (self->manager, extension);
    else
        exm_manager_disable_extension (self->manager, extension);
}

typedef struct
{
    ExmManager *manager;
    ExmExtension *extension;
} RemoveDialogData;

static void
extension_remove_dialog_response (GtkDialog        *dialog,
                                  int               response_id,
                                  RemoveDialogData *data)
{
    gtk_window_destroy (GTK_WINDOW (dialog));

    if (response_id == GTK_RESPONSE_YES)
    {
        exm_manager_remove_extension (data->manager, data->extension);
    }

    g_clear_pointer (&data->manager, g_object_unref);
    g_clear_pointer (&data->extension, g_object_unref);
    g_free (data);
}

static void
extension_remove (GtkWidget  *widget,
                  const char *action_name,
                  GVariant   *param)
{
    ExmWindow *self;
    ExmExtension *extension;
    gchar *uuid;

    self = EXM_WINDOW (widget);
    g_variant_get (param, "s", &uuid);

    extension = exm_manager_get_by_uuid (self->manager, uuid);

    GtkWidget *dlg;

    dlg = gtk_message_dialog_new (GTK_WINDOW (self),
                                  GTK_DIALOG_MODAL,
                                  GTK_MESSAGE_QUESTION,
                                  GTK_BUTTONS_YES_NO,
                                  _("Are you sure you want to uninstall?"));

    RemoveDialogData *data = g_new0 (RemoveDialogData, 1);
    data->manager = g_object_ref (self->manager);
    data->extension = g_object_ref (extension);

    g_signal_connect (dlg, "response", G_CALLBACK (extension_remove_dialog_response), data);
    gtk_widget_show (dlg);
}

static void
on_install_done (GObject       *source,
                 GAsyncResult  *res)
{
    GError *error = NULL;
    if (!exm_manager_install_finish (EXM_MANAGER (source), res, &error) && error)
    {
        // TODO: Properly log this
        g_critical ("%s\n", error->message);
    }
}

static void
extension_install (GtkWidget  *widget,
                   const char *action_name,
                   GVariant   *param)
{
    ExmWindow *self;
    gchar *uuid;

    self = EXM_WINDOW (widget);
    g_variant_get (param, "s", &uuid);

    exm_manager_install_async (self->manager, uuid, NULL,
                               (GAsyncReadyCallback) on_install_done,
                               self);
}

static void
show_view (GtkWidget  *widget,
           const char *action_name,
           GVariant   *param)
{
    ExmWindow *self;

    self = EXM_WINDOW (widget);

    if (g_str_equal (action_name, "win.show-detail"))
    {
        gchar *uuid;

        g_variant_get (param, "s", &uuid);
        adw_leaflet_set_visible_child (self->leaflet, self->detail_view);
        return;
    }

    adw_leaflet_set_visible_child (self->leaflet, self->main_view);
}

static void
exm_window_class_init (ExmWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_window_finalize;
    object_class->get_property = exm_window_get_property;
    object_class->set_property = exm_window_set_property;

    properties [PROP_MANAGER]
        = g_param_spec_object ("manager",
                               "Manager",
                               "Manager",
                               EXM_TYPE_MANAGER,
                               G_PARAM_READABLE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-window.ui");
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, header_bar);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, global_toggle);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, installed_page);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, browse_page);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, leaflet);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, main_view);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, detail_view);

    // TODO: Refactor ExmWindow into a separate ExmController and supply the
    // necessary actions/methods/etc in there. A reference to this new object can
    // then be passed to each page.
    gtk_widget_class_install_action (widget_class, "ext.install", "s", extension_install);
    gtk_widget_class_install_action (widget_class, "ext.remove", "s", extension_remove);
    gtk_widget_class_install_action (widget_class, "ext.state-set", "(sb)", extension_state_set);
    gtk_widget_class_install_action (widget_class, "ext.open-prefs", "s", extension_open_prefs);
    gtk_widget_class_install_action (widget_class, "win.show-detail", "s", show_view);
    gtk_widget_class_install_action (widget_class, "win.show-main", NULL, show_view);
}

static void
exm_window_init (ExmWindow *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    self->manager = exm_manager_new ();

    g_object_set (self->installed_page, "manager", self->manager, NULL);
    g_object_set (self->browse_page, "manager", self->manager, NULL);

    g_object_bind_property (self->manager,
                            "extensions-enabled",
                            self->global_toggle,
                            "state",
                            G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
}
