/* exm-window.c
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

#include "exm-window.h"

#include "exm-browse-page.h"
#include "exm-config.h"
#include "exm-detail-view.h"
#include "exm-error-dialog.h"
#include "exm-installed-page.h"
#include "exm-screenshot-view.h"
#include "exm-upgrade-assistant.h"
#include "local/exm-extension.h"
#include "local/exm-manager.h"

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
    ExmScreenshotView    *screenshot_view;
    AdwViewSwitcher      *title;
    GtkToggleButton      *search_button;
    GtkSearchBar         *search_bar;
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

GtkSearchBar *
exm_window_get_search_bar (ExmWindow *self)
{
  return self->search_bar;
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
search_online (GtkWidget  *widget,
               const char *action_name,
               GVariant   *param)
{
    ExmWindow *self;
    GtkSearchEntry *search_entry;
    const char *search_text;

    self = EXM_WINDOW (widget);

    search_entry = exm_browse_page_get_search_entry (self->browse_page);
    search_text = gtk_editable_get_text (GTK_EDITABLE (gtk_search_bar_get_child (self->search_bar)));
    adw_view_stack_set_visible_child_name (self->view_stack, "browse");
    gtk_editable_set_text (GTK_EDITABLE (search_entry) , search_text);
    gtk_toggle_button_set_active (self->search_button, FALSE);
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
on_visible_page_changed (AdwViewStack *view_stack,
                         GtkWidget    *widget,
                         ExmWindow    *self)
{
    gboolean is_installed_page = EXM_IS_INSTALLED_PAGE (adw_view_stack_get_visible_child (view_stack));

    adw_view_stack_page_set_needs_attention (self->installed_stack,
                                             !is_installed_page &&
                                             adw_view_stack_page_get_badge_number (self->installed_stack) > 0);

    gtk_widget_set_visible (GTK_WIDGET (self->search_bar),
                            is_installed_page);

    gtk_widget_set_visible (GTK_WIDGET (self->search_button),
                            is_installed_page);
}

void
on_visible_installed_stack_changed (GObject    *object,
                                    GParamSpec *pspec,
                                    gpointer    user_data)
{
    ExmWindow *self = (ExmWindow *) user_data;
    gboolean search_mode;
    GtkFilterListModel *search_list_model;
    GtkStack *stack;

    search_mode = gtk_search_bar_get_search_mode (self->search_bar);
    search_list_model = exm_installed_page_get_search_list_model (self->installed_page);
    stack = exm_installed_page_get_stack (self->installed_page);

    if (search_mode && g_list_model_get_n_items (G_LIST_MODEL (search_list_model)) > 0)
        gtk_stack_set_visible_child_name (stack , "page_results");
    else if (search_mode)
        gtk_stack_set_visible_child_name (stack , "page_empty");
    else
        gtk_stack_set_visible_child_name (stack , "page_list");
}

static gboolean
search_open_cb (GtkWidget *widget,
                GVariant  *args,
                gpointer   user_data)
{
  ExmWindow *self = EXM_WINDOW (widget);
  AdwNavigationPage *visible_page;
  const char *visible_stack_name, *visible_page_name;

  visible_page = adw_navigation_view_get_visible_page (self->navigation_view);
  visible_stack_name = adw_view_stack_get_visible_child_name (self->view_stack);
  visible_page_name = adw_navigation_page_get_tag (visible_page);

  if (g_strcmp0 (visible_stack_name, "installed") == 0
      && g_strcmp0 (visible_page_name, "main") == 0)
  {
      gboolean is_active = gtk_toggle_button_get_active (self->search_button);
      gtk_toggle_button_set_active (self->search_button, !is_active);
  }
  else if (g_strcmp0 (visible_stack_name, "browse") == 0)
  {
      GtkSearchEntry *search_entry = exm_browse_page_get_search_entry (self->browse_page);
      gtk_widget_grab_focus (GTK_WIDGET (search_entry));
  }
  else
  {
      return GDK_EVENT_PROPAGATE;
  }

  return GDK_EVENT_STOP;
}

static void
screenshot_zoom (GtkWidget  *widget,
                 const char *action_name,
                 GVariant   *parameter)
{
  ExmWindow *self = (ExmWindow *)widget;

  g_assert (EXM_IS_WINDOW (self));

  exm_screenshot_view_zoom (self->screenshot_view, action_name);
}

static void
exm_window_class_init (ExmWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = exm_window_get_property;

    properties [PROP_MANAGER]
        = g_param_spec_object ("manager",
                               "Manager",
                               "Manager",
                               EXM_TYPE_MANAGER,
                               G_PARAM_READABLE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-window.ui", RESOURCE_PATH));
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, header_bar);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, installed_page);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, browse_page);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, navigation_view);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, main_view);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, detail_view);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, screenshot_view);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, title);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, search_button);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, search_bar);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, view_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, installed_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, toast_overlay);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, remove_dialog);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, unsupported_dialog);

    gtk_widget_class_bind_template_callback (widget_class, on_visible_page_changed);
    gtk_widget_class_bind_template_callback (widget_class, on_visible_installed_stack_changed);

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
    gtk_widget_class_install_action (widget_class, "win.search-online", NULL, search_online);
    gtk_widget_class_install_action (widget_class, "screenshot.zoom-in", NULL, screenshot_zoom);
    gtk_widget_class_install_action (widget_class, "screenshot.zoom-out", NULL, screenshot_zoom);
    gtk_widget_class_install_action (widget_class, "screenshot.zoom-reset", NULL, screenshot_zoom);

    gtk_widget_class_add_binding (widget_class, GDK_KEY_f, GDK_CONTROL_MASK, search_open_cb, NULL);

    gtk_widget_class_add_binding_action (widget_class, GDK_KEY_1, GDK_ALT_MASK, "win.show-page", "s", "installed");
    gtk_widget_class_add_binding_action (widget_class, GDK_KEY_2, GDK_ALT_MASK, "win.show-page", "s", "browse");
    gtk_widget_class_add_binding_action (widget_class, GDK_KEY_plus, GDK_CONTROL_MASK, "screenshot.zoom-in", NULL);
    gtk_widget_class_add_binding_action (widget_class, GDK_KEY_minus, GDK_CONTROL_MASK, "screenshot.zoom-out", NULL);
    gtk_widget_class_add_binding_action (widget_class, GDK_KEY_0, GDK_CONTROL_MASK, "screenshot.zoom-reset", NULL);
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
