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
#include "exm-detail-view.h"
#include "exm-screenshot-view.h"
#include "exm-upgrade-assistant.h"
#include "exm-error-dialog.h"

#include "local/exm-manager.h"
#include "local/exm-extension.h"

#include <glib/gi18n.h>

struct _ExmWindow
{
    AdwApplicationWindow  parent_instance;

    ExmManager *manager;

    /* Template widgets */
    AdwHeaderBar         *header_bar;
    ExmBrowsePage        *browse_page;
    ExmInstalledPage     *installed_page;
    AdwNavigationView    *navigation_view;
    AdwNavigationPage    *main_view;
    ExmDetailView        *detail_view;
    AdwViewSwitcher      *title;
    AdwViewStack         *view_stack;
    AdwViewStackPage     *installed_stack;
    AdwToastOverlay      *toast_overlay;
    AdwAlertDialog       *remove_dialog;
    AdwAlertDialog       *unsupported_dialog;
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

typedef struct
{
    ExmManager *manager;
    ExmExtension *extension;
} RemoveDialogData;

static void
extension_remove_dialog_response (AdwAlertDialog   *dialog,
                                  GAsyncResult     *result,
                                  RemoveDialogData *data)
{
    const char *response = adw_alert_dialog_choose_finish (dialog, result);

    if (strcmp(response, "yes") == 0)
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

    RemoveDialogData *data = g_new0 (RemoveDialogData, 1);
    data->manager = g_object_ref (self->manager);
    data->extension = g_object_ref (extension);

    adw_alert_dialog_choose (self->remove_dialog, widget, NULL,
                             (GAsyncReadyCallback) extension_remove_dialog_response, data);
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

typedef struct
{
    ExmManager *manager;
    gchar *uuid;
} UnsupportedDialogData;

static void
extension_unsupported_dialog_response (AdwAlertDialog        *dialog,
                                       GAsyncResult          *result,
                                       UnsupportedDialogData *data)
{
    const char *response = adw_alert_dialog_choose_finish (dialog, result);

    if (strcmp(response, "install") == 0)
    {
        exm_manager_install_async (data->manager, data->uuid, NULL,
                                   (GAsyncReadyCallback) on_install_done,
                                   NULL);
    }

    g_clear_pointer (&data->manager, g_object_unref);
    g_clear_pointer (&data->uuid, g_free);
    g_free (data);
}

static void
extension_install (GtkWidget  *widget,
                   const char *action_name,
                   GVariant   *param)
{
    ExmWindow *self;
    gchar *uuid;
    gboolean warn;

    self = EXM_WINDOW (widget);
    g_variant_get (param, "(sb)", &uuid, &warn);

    if (warn)
    {
        UnsupportedDialogData *data = g_new0 (UnsupportedDialogData, 1);
        data->manager = g_object_ref (self->manager);
        data->uuid = g_strdup (uuid);

        adw_alert_dialog_choose (self->unsupported_dialog, widget, NULL,
                                 (GAsyncReadyCallback) extension_unsupported_dialog_response, data);

        return;
    }

    exm_manager_install_async (self->manager, uuid, NULL,
                               (GAsyncReadyCallback) on_install_done,
                               self);
}

static void
show_page (GtkWidget  *widget,
           const char *action_name,
           GVariant   *param)
{
    ExmWindow *self;
    char *target;

    g_variant_get (param, "s", &target);

    self = EXM_WINDOW (widget);

    adw_view_stack_set_visible_child_name (self->view_stack, target);
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
        adw_navigation_page_set_title (ADW_NAVIGATION_PAGE (self->detail_view), uuid);
        adw_navigation_view_push (self->navigation_view, ADW_NAVIGATION_PAGE (self->detail_view));

        exm_detail_view_load_for_uuid (self->detail_view, uuid);

        return;
    }

    adw_navigation_view_pop_to_page (self->navigation_view, self->main_view);
}

static void
show_upgrade_assistant (GtkWidget  *widget,
                        const char *action_name,
                        GVariant   *param)
{
    ExmWindow *self;

    self = EXM_WINDOW (widget);

    ExmUpgradeAssistant *assistant = exm_upgrade_assistant_new (self->manager);
    adw_dialog_present (ADW_DIALOG (assistant), widget);
}

static void
show_error_dialog (GtkWidget  *widget,
                   const char *action_name,
                   GVariant   *param)
{
    ExmErrorDialog *err_dialog;
    const char *err_text;

    err_text = g_variant_get_string (param, NULL);
    err_dialog = exm_error_dialog_new (err_text);

    gtk_window_set_modal (GTK_WINDOW (err_dialog), TRUE);
    gtk_window_set_transient_for (GTK_WINDOW (err_dialog), GTK_WINDOW (widget));

    gtk_window_present (GTK_WINDOW (err_dialog));
}

static void
show_error (GtkWidget  *widget,
            const char *action_name,
            GVariant   *param)
{
    ExmWindow *self;
    char *error_text;
    AdwToast *toast;

    self = EXM_WINDOW (widget);

    g_variant_get (param, "s", &error_text);

    toast = adw_toast_new (_("An Error Occurred"));
    adw_toast_set_button_label (toast, _("_Details"));

    adw_toast_set_action_name (toast, "win.show-error-dialog");
    adw_toast_set_action_target (toast, "s", error_text);

    adw_toast_overlay_add_toast (self->toast_overlay, toast);
}


static void
on_error (ExmManager *manager,
          char       *error_text,
          ExmWindow  *self)
{
    gtk_widget_activate_action (GTK_WIDGET (self), "win.show-error", "s", error_text);
}

static void
on_updates_available (ExmManager *manager,
                      int         n_updates,
                      ExmWindow  *self)
{
    adw_view_stack_page_set_badge_number (self->installed_stack, (guint) n_updates);
}

static void
on_needs_attention (AdwViewStack *view_stack,
                    GtkWidget    *widget,
                    ExmWindow    *self)
{
    adw_view_stack_page_set_needs_attention (self->installed_stack,
                                             EXM_IS_BROWSE_PAGE (adw_view_stack_get_visible_child (view_stack))
                                             && adw_view_stack_page_get_badge_number (self->installed_stack) > 0);
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
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, installed_page);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, browse_page);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, navigation_view);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, main_view);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, detail_view);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, title);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, view_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, installed_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, toast_overlay);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, remove_dialog);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, unsupported_dialog);

    gtk_widget_class_bind_template_callback (widget_class, on_needs_attention);

    // TODO: Refactor ExmWindow into a separate ExmController and supply the
    // necessary actions/methods/etc in there. A reference to this new object can
    // then be passed to each page.
    gtk_widget_class_install_action (widget_class, "ext.install", "(sb)", extension_install);
    gtk_widget_class_install_action (widget_class, "ext.remove", "s", extension_remove);
    gtk_widget_class_install_action (widget_class, "ext.open-prefs", "s", extension_open_prefs);
    gtk_widget_class_install_action (widget_class, "win.show-detail", "s", show_view);
    gtk_widget_class_install_action (widget_class, "win.show-main", NULL, show_view);
    gtk_widget_class_install_action (widget_class, "win.show-upgrade-assistant", NULL, show_upgrade_assistant);
    gtk_widget_class_install_action (widget_class, "win.show-page", "s", show_page);
    gtk_widget_class_install_action (widget_class, "win.show-error", "s", show_error);
    gtk_widget_class_install_action (widget_class, "win.show-error-dialog", "s", show_error_dialog);
}

static void
exm_window_init (ExmWindow *self)
{
    g_type_ensure (EXM_TYPE_INSTALLED_PAGE);
    g_type_ensure (EXM_TYPE_BROWSE_PAGE);
    g_type_ensure (EXM_TYPE_DETAIL_VIEW);
    g_type_ensure (EXM_TYPE_SCREENSHOT_VIEW);

    gtk_widget_init_template (GTK_WIDGET (self));

    if (strstr (APP_ID, ".Devel") != NULL) {
        gtk_widget_add_css_class (GTK_WIDGET (self), "devel");
    }

    self->manager = exm_manager_new ();
    g_signal_connect (self->manager, "error-occurred", G_CALLBACK (on_error), self);

    g_object_set (self->installed_page, "manager", self->manager, NULL);
    g_object_set (self->browse_page, "manager", self->manager, NULL);
    g_object_set (self->detail_view, "manager", self->manager, NULL);

    g_object_bind_property (self->manager,
                            "shell-version",
                            self->detail_view,
                            "shell-version",
                            G_BINDING_SYNC_CREATE);

    g_signal_connect (self->manager,
                      "updates-available",
                      G_CALLBACK (on_updates_available),
                      self);
}
