/*
 * exm-window.c
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

#include "exm-window.h"

#include "exm-author-page.h"
#include "exm-browse-page.h"
#include "exm-config.h"
#include "exm-detail-view.h"
#include "exm-error-dialog.h"
#include "exm-extension-error-dialog.h"
#include "exm-installed-page.h"
#include "exm-screenshot-view.h"
#include "exm-types.h"
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
    GtkStack             *main_stack;
    AdwNavigationView    *navigation_view;
    AdwNavigationPage    *main_view;
    ExmScreenshotView    *screenshot_view;
    AdwViewSwitcher      *title;
    GtkToggleButton      *search_button;
    GtkSearchBar         *search_bar;
    AdwViewStack         *view_stack;
    AdwViewStackPage     *installed_stack;
    AdwToastOverlay      *toast_overlay;
    AdwAlertDialog       *remove_dialog;
    AdwAlertDialog       *unsupported_dialog;
    AdwAlertDialog       *select_remove_dialog;
};

G_DEFINE_TYPE (ExmWindow, exm_window, ADW_TYPE_APPLICATION_WINDOW)

enum {
    PROP_0,
    PROP_MANAGER,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

enum {
    SIGNAL_0,
    SIGNAL_SEARCH_CHANGED,
    N_SIGNALS
};

static guint signals [N_SIGNALS];

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
    case PROP_MANAGER:
        // Borrowed reference: the manager is owned by ExmApplication and
        // outlives every window.
        self->manager = g_value_get_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
extension_open_prefs (GtkWidget  *widget,
                      const char *action_name G_GNUC_UNUSED,
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
extension_show_error (GtkWidget  *widget,
                      const char *action_name G_GNUC_UNUSED,
                      GVariant   *param)
{
    ExmWindow *self;
    ExmExtension *extension;
    ExmExtensionErrorDialog *dialog;
    gchar *uuid;
    gchar *name = NULL;
    gchar *error_text = NULL;
    gchar *homepage = NULL;

    self = EXM_WINDOW (widget);
    g_variant_get (param, "s", &uuid);

    extension = exm_manager_get_by_uuid (self->manager, uuid);

    if (!extension)
        return;

    g_object_get (extension, "name", &name, "error", &error_text, "url", &homepage, NULL);

    if (error_text && error_text[0])
    {
        dialog = exm_extension_error_dialog_new (name, error_text,
                                                 (homepage && homepage[0]) ? homepage : NULL);
        adw_dialog_present (ADW_DIALOG (dialog), widget);
    }

    g_free (name);
    g_free (homepage);
    g_free (error_text);
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

    if (g_strcmp0 (response, "yes") == 0)
        exm_manager_remove_extension (data->manager, data->extension);

    g_clear_pointer (&data->manager, g_object_unref);
    g_clear_pointer (&data->extension, g_object_unref);
    g_free (data);
}

static void
extension_remove (GtkWidget  *widget,
                  const char *action_name G_GNUC_UNUSED,
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

typedef struct
{
    ExmManager *manager;
    gchar *uuid;
} UnsupportedDialogData;

static void
on_install_done (GObject      *source,
                 GAsyncResult *res,
                 gpointer      user_data)
{
    ExmWindow *self = NULL;
    UnsupportedDialogData *data = NULL;
    ExmManager *manager = NULL;
    ExmInstallButtonState state;
    GError *error = NULL;

    if (G_IS_OBJECT (user_data))
    {
        self = (ExmWindow *)user_data;
        manager = self->manager;
        state = EXM_INSTALL_BUTTON_STATE_DEFAULT;
    }
    else
    {
        data = (UnsupportedDialogData *)user_data;
        manager = data->manager;
        state = EXM_INSTALL_BUTTON_STATE_UNSUPPORTED;
    }

    if (!exm_manager_install_finish (source, res, &error))
    {
        // TODO: Properly log this
        if (error)
        {
            g_critical ("%s\n", error->message);
            g_clear_error (&error);
        }

        g_signal_emit_by_name (manager, "install-status", state);
    }
    else
    {
        g_signal_emit_by_name (manager, "install-status", EXM_INSTALL_BUTTON_STATE_INSTALLED);
    }

    if (data)
    {
        g_clear_pointer (&data->manager, g_object_unref);
        g_clear_pointer (&data->uuid, g_free);
        g_free (data);
    }
}

static void
extension_unsupported_dialog_response (AdwAlertDialog        *dialog,
                                       GAsyncResult          *result,
                                       UnsupportedDialogData *data)
{
    const char *response = adw_alert_dialog_choose_finish (dialog, result);

    if (g_strcmp0 (response, "install") == 0)
    {
        exm_manager_install_async (data->manager, data->uuid, NULL,
                                   (GAsyncReadyCallback) on_install_done,
                                   data);
    }
    else
    {
        g_signal_emit_by_name (data->manager, "install-status", EXM_INSTALL_BUTTON_STATE_UNSUPPORTED);

        g_clear_pointer (&data->manager, g_object_unref);
        g_clear_pointer (&data->uuid, g_free);
        g_free (data);
    }
}

static void
extension_install (GtkWidget  *widget,
                   const char *action_name G_GNUC_UNUSED,
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
        g_free (uuid);

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
           const char *action_name G_GNUC_UNUSED,
           GVariant   *param)
{
    ExmWindow *self;
    char *target;

    g_variant_get (param, "s", &target);

    self = EXM_WINDOW (widget);

    adw_view_stack_set_visible_child_name (self->view_stack, target);
}

static ExmDetailView *
create_detail_view (ExmWindow   *self,
                    const gchar *title)
{
    ExmDetailView *view = exm_detail_view_new ();

    adw_navigation_page_set_title (ADW_NAVIGATION_PAGE (view), title);

    g_object_set (view, "manager", self->manager, NULL);
    g_object_bind_property (self->manager, "shell-version", view, "shell-version",
                            G_BINDING_SYNC_CREATE);

    adw_navigation_view_add (self->navigation_view, ADW_NAVIGATION_PAGE (view));

    return view;
}

static ExmAuthorPage *
create_author_page (ExmWindow   *self,
                    const gchar *title)
{
    ExmAuthorPage *page = exm_author_page_new ();

    adw_navigation_page_set_title (ADW_NAVIGATION_PAGE (page), title);

    g_object_set (page, "manager", self->manager, NULL);

    adw_navigation_view_add (self->navigation_view, ADW_NAVIGATION_PAGE (page));

    return page;
}

static void
show_view (GtkWidget  *widget,
           const char *action_name,
           GVariant   *param)
{
    ExmWindow *self;

    self = EXM_WINDOW (widget);

    if (g_strcmp0 (action_name, "win.show-detail") == 0)
    {
        gchar *uuid;
        ExmDetailView *view;

        g_variant_get (param, "s", &uuid);

        view = create_detail_view (self, uuid);
        adw_navigation_view_push (self->navigation_view, ADW_NAVIGATION_PAGE (view));

        exm_detail_view_load_for_uuid (view, uuid);

        return;
    }
    else if (g_strcmp0 (action_name, "win.show-author") == 0)
    {
        guint32 creator_id;
        const gchar *username;
        const gchar *display_name;
        ExmAuthorPage *page;

        g_variant_get (param, "(u&s&s)", &creator_id, &username, &display_name);

        page = create_author_page (self, display_name);
        adw_navigation_view_push (self->navigation_view, ADW_NAVIGATION_PAGE (page));

        exm_author_page_load (page, creator_id, username, display_name);

        return;
    }
    else if (g_strcmp0 (action_name, "win.show-screenshot") == 0)
    {
        AdwNavigationPage *visible = adw_navigation_view_get_visible_page (self->navigation_view);

        if (EXM_IS_DETAIL_VIEW (visible))
        {
            GdkPaintable *screenshot = NULL;

            g_object_get (visible, "screenshot", &screenshot, NULL);
            adw_navigation_page_set_title (ADW_NAVIGATION_PAGE (self->screenshot_view),
                                           adw_navigation_page_get_title (visible));
            g_object_set (self->screenshot_view, "screenshot", screenshot, NULL);
            g_clear_object (&screenshot);
        }

        adw_navigation_view_push (self->navigation_view, ADW_NAVIGATION_PAGE (self->screenshot_view));

        return;
    }

    adw_navigation_view_pop_to_page (self->navigation_view, self->main_view);
}

static void
show_upgrade_assistant (GtkWidget  *widget,
                        const char *action_name G_GNUC_UNUSED,
                        GVariant   *param G_GNUC_UNUSED)
{
    ExmWindow *self;

    self = EXM_WINDOW (widget);

    ExmUpgradeAssistant *assistant = exm_upgrade_assistant_new (self->manager);
    adw_dialog_present (ADW_DIALOG (assistant), widget);
}

static void
show_error_dialog (GtkWidget  *widget,
                   const char *action_name G_GNUC_UNUSED,
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
            const char *action_name G_GNUC_UNUSED,
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
discover_gnome (GtkWidget  *widget,
                const char *action_name G_GNUC_UNUSED,
                GVariant   *param G_GNUC_UNUSED)
{
    GtkUriLauncher *launcher;

    launcher = gtk_uri_launcher_new ("https://www.gnome.org");
    gtk_uri_launcher_launch (launcher, GTK_WINDOW (widget), NULL, NULL, NULL);
    g_object_unref (launcher);
}

static void
search_online (GtkWidget  *widget,
               const char *action_name G_GNUC_UNUSED,
               GVariant   *param G_GNUC_UNUSED)
{
    ExmWindow *self;
    const char *search_text;

    self = EXM_WINDOW (widget);

    search_text = gtk_editable_get_text (GTK_EDITABLE (gtk_search_bar_get_child (self->search_bar)));
    adw_view_stack_set_visible_child_name (self->view_stack, "browse");
    exm_browse_page_search (self->browse_page, search_text);
    gtk_toggle_button_set_active (self->search_button, FALSE);
}

static void
search_installed (GtkWidget  *widget,
                  const char *action_name G_GNUC_UNUSED,
                  GVariant   *param)
{
    ExmWindow *self;
    const char *search_text;

    self = EXM_WINDOW (widget);

    adw_navigation_view_pop_to_page (self->navigation_view, self->main_view);
    adw_view_stack_set_visible_child_name (self->view_stack, "installed");

    search_text = g_variant_get_string (param, NULL);
    gtk_editable_set_text (GTK_EDITABLE (gtk_search_bar_get_child (self->search_bar)), search_text);
    gtk_toggle_button_set_active (self->search_button, TRUE);
}


static void
on_error (ExmManager *manager G_GNUC_UNUSED,
          char       *error_text,
          ExmWindow  *self)
{
    gtk_widget_activate_action (GTK_WIDGET (self), "win.show-error", "s", error_text);
}

static void
sync_visible_page (ExmWindow *self)
{
    gboolean is_supported;

    g_object_get (self->manager, "is-supported", &is_supported, NULL);

    gtk_stack_set_visible_child_name (self->main_stack,
                                      is_supported ? "app" : "noshell");
}

static void
on_updates_available (ExmManager *manager G_GNUC_UNUSED,
                      int         n_updates,
                      ExmWindow  *self)
{
    adw_view_stack_page_set_badge_number (self->installed_stack, (guint) n_updates);
}

static gchar *
selection_mode_page_name (GObject  *object G_GNUC_UNUSED,
                          gboolean  selection_mode)
{
    return g_strdup (selection_mode ? "selection" : "switcher");
}

static gchar *
selection_title_text (GObject *object G_GNUC_UNUSED,
                      guint    n_selected)
{
    if (n_selected == 0)
        return g_strdup (_("Select Extensions"));

    // Translators: "%u" = number of selected extensions; shown as the header bar title
    return g_strdup_printf (ngettext ("%u Selected", "%u Selected", n_selected), n_selected);
}

static gboolean
has_selection (GObject *object G_GNUC_UNUSED,
               guint    n_selected)
{
    return n_selected > 0;
}

static void
on_cancel_selection_clicked (GtkButton *btn G_GNUC_UNUSED,
                             ExmWindow *self)
{
    g_object_set (self->installed_page, "selection-mode", FALSE, NULL);
}

typedef struct {
    ExmWindow *window;
} SelectRemoveData;

static void
on_uninstall_confirm_response (AdwAlertDialog  *dialog G_GNUC_UNUSED,
                               GAsyncResult    *result,
                               SelectRemoveData *data)
{
    const char *response = adw_alert_dialog_choose_finish (data->window->select_remove_dialog,
                                                           result);
    if (g_strcmp0 (response, "yes") == 0)
    {
        exm_installed_page_uninstall_selected (data->window->installed_page);
        g_object_set (data->window->installed_page, "selection-mode", FALSE, NULL);
    }
    g_free (data);
}

static void
on_uninstall_selected_clicked (GtkButton *btn G_GNUC_UNUSED,
                               ExmWindow *self)
{
    guint n;
    char *body;
    SelectRemoveData *data;

    g_object_get (self->installed_page, "n-selected", &n, NULL);

    body = g_strdup_printf (
        ngettext ("The extension's features and functionality will no longer "
                  "be accessible. Are you sure you want to uninstall %u extension?",
                  "Their features and functionality will no longer be "
                  "accessible. Are you sure you want to uninstall %u extensions?", n), n);
    adw_alert_dialog_set_body (self->select_remove_dialog, body);
    g_free (body);

    data = g_new0 (SelectRemoveData, 1);
    data->window = self;

    adw_alert_dialog_choose (self->select_remove_dialog, GTK_WIDGET (self), NULL,
                             (GAsyncReadyCallback) on_uninstall_confirm_response, data);
}

static void
on_visible_page_changed (AdwViewStack *view_stack,
                         GtkWidget    *widget G_GNUC_UNUSED,
                         ExmWindow    *self)
{
    gboolean is_installed_page = EXM_IS_INSTALLED_PAGE (adw_view_stack_get_visible_child (view_stack));
    gboolean selection_mode;

    g_object_get (self->installed_page, "selection-mode", &selection_mode, NULL);

    if (!is_installed_page && selection_mode)
    {
        g_object_set (self->installed_page, "selection-mode", FALSE, NULL);
        selection_mode = FALSE;
    }

    adw_view_stack_page_set_needs_attention (self->installed_stack,
                                             !is_installed_page &&
                                             adw_view_stack_page_get_badge_number (self->installed_stack) > 0);

    gtk_widget_set_visible (GTK_WIDGET (self->search_bar),
                            is_installed_page);

    gtk_widget_set_visible (GTK_WIDGET (self->search_button),
                            is_installed_page);
}

static gboolean
search_open_cb (GtkWidget *widget,
                GVariant  *args G_GNUC_UNUSED,
                gpointer   user_data G_GNUC_UNUSED)
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
      exm_browse_page_focus_entry (self->browse_page);
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
                 GVariant   *parameter G_GNUC_UNUSED)
{
  ExmWindow *self = (ExmWindow *)widget;

  g_assert (EXM_IS_WINDOW (self));

  exm_screenshot_view_zoom (self->screenshot_view, action_name);
}

static void
exm_window_constructed (GObject *object)
{
    ExmWindow *self = EXM_WINDOW (object);

    G_OBJECT_CLASS (exm_window_parent_class)->constructed (object);

    g_signal_connect (self->manager, "error-occurred", G_CALLBACK (on_error), self);
    g_signal_connect_swapped (self->manager, "notify::is-supported",
                              G_CALLBACK (sync_visible_page), self);
    sync_visible_page (self);

    g_object_set (self->installed_page, "manager", self->manager, NULL);
    g_object_set (self->browse_page, "manager", self->manager, NULL);

    g_signal_connect (self->manager,
                      "updates-available",
                      G_CALLBACK (on_updates_available),
                      self);
}

static void
exm_window_class_init (ExmWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = exm_window_get_property;
    object_class->set_property = exm_window_set_property;
    object_class->constructed = exm_window_constructed;

    properties [PROP_MANAGER]
        = g_param_spec_object ("manager",
                               "Manager",
                               "Manager",
                               EXM_TYPE_MANAGER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    signals [SIGNAL_SEARCH_CHANGED]
        = g_signal_new ("search-changed",
                        G_TYPE_FROM_CLASS (object_class),
                        G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE|G_SIGNAL_NO_HOOKS,
                        0, NULL, NULL, NULL,
                        G_TYPE_NONE, 0,
                        NULL);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-window.ui", RESOURCE_PATH));
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, header_bar);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, installed_page);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, browse_page);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, main_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, navigation_view);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, main_view);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, screenshot_view);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, title);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, search_button);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, search_bar);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, view_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, installed_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, toast_overlay);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, remove_dialog);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, unsupported_dialog);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, select_remove_dialog);

    gtk_widget_class_bind_template_callback (widget_class, on_visible_page_changed);
    gtk_widget_class_bind_template_callback (widget_class, on_cancel_selection_clicked);
    gtk_widget_class_bind_template_callback (widget_class, on_uninstall_selected_clicked);
    gtk_widget_class_bind_template_callback (widget_class, selection_mode_page_name);
    gtk_widget_class_bind_template_callback (widget_class, selection_title_text);
    gtk_widget_class_bind_template_callback (widget_class, has_selection);

    // TODO: Refactor ExmWindow into a separate ExmController and supply the
    // necessary actions/methods/etc in there. A reference to this new object can
    // then be passed to each page.
    gtk_widget_class_install_action (widget_class, "ext.install", "(sb)", extension_install);
    gtk_widget_class_install_action (widget_class, "ext.remove", "s", extension_remove);
    gtk_widget_class_install_action (widget_class, "ext.open-prefs", "s", extension_open_prefs);
    gtk_widget_class_install_action (widget_class, "ext.show-error", "s", extension_show_error);
    gtk_widget_class_install_action (widget_class, "win.show-detail", "s", show_view);
    gtk_widget_class_install_action (widget_class, "win.show-author", "(uss)", show_view);
    gtk_widget_class_install_action (widget_class, "win.show-main", NULL, show_view);
    gtk_widget_class_install_action (widget_class, "win.show-screenshot", NULL, show_view);
    gtk_widget_class_install_action (widget_class, "win.show-upgrade-assistant", NULL, show_upgrade_assistant);
    gtk_widget_class_install_action (widget_class, "win.show-page", "s", show_page);
    gtk_widget_class_install_action (widget_class, "win.show-error", "s", show_error);
    gtk_widget_class_install_action (widget_class, "win.show-error-dialog", "s", show_error_dialog);
    gtk_widget_class_install_action (widget_class, "win.search-online", NULL, search_online);
    gtk_widget_class_install_action (widget_class, "win.search-installed", "s", search_installed);
    gtk_widget_class_install_action (widget_class, "win.discover-gnome", NULL, discover_gnome);
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
    g_type_ensure (EXM_TYPE_SCREENSHOT_VIEW);

    gtk_widget_init_template (GTK_WIDGET (self));

    if (strstr (APP_ID, ".Devel") != NULL)
        gtk_widget_add_css_class (GTK_WIDGET (self), "devel");
}
