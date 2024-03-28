/* exm-detail-view.c
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

#include "exm-detail-view.h"

#include "exm-screenshot.h"
#include "exm-screenshot-view.h"
#include "exm-info-bar.h"
#include "exm-comment-tile.h"
#include "exm-comment-dialog.h"
#include "exm-unified-data.h"

#include "web/exm-data-provider.h"
#include "web/exm-image-resolver.h"
#include "web/exm-comment-provider.h"
#include "web/model/exm-shell-version-map.h"
#include "web/model/exm-comment.h"
#include "local/exm-manager.h"

#include "exm-types.h"
#include "exm-enums.h"

#include <glib/gi18n.h>

struct _ExmDetailView
{
    AdwNavigationPage parent_instance;

    ExmUnifiedData *data;
    ExmExtension *extension;

    ExmManager *manager;
    ExmDataProvider *provider;
    ExmImageResolver *resolver;
    ExmCommentProvider *comment_provider;
    GCancellable *resolver_cancel;

  	GSimpleAction *zoom_in;
    GSimpleAction *zoom_out;
    GSimpleAction *zoom_reset;

    GSimpleActionGroup *action_group;

    gchar *shell_version;
    gchar *uuid;

    AdwHeaderBar *header_bar;
    AdwWindowTitle *title;
    GtkStack *stack;
    GtkButton *ext_install;
    GtkLabel *ext_description;
    GtkImage *ext_icon;
    GtkLabel *ext_title;
    GtkLabel *ext_author;
    ExmScreenshot *ext_screenshot;
	GtkOverlay *ext_screenshot_container;
	GtkButton *ext_screenshot_popout_button;
    ExmInfoBar *ext_info_bar;
    GtkScrolledWindow *scroll_area;
    GtkStack *comment_stack;
    GtkFlowBox *comment_box;
    GtkButton *show_more_btn;
    GtkStack *tools_stack;
    GtkSwitch *ext_toggle;

    AdwActionRow *link_homepage;
    gchar *uri_homepage;
    AdwActionRow *link_extensions;
    gchar *uri_extensions;
    int pk;
    guint signal_id;
};

G_DEFINE_FINAL_TYPE (ExmDetailView, exm_detail_view, ADW_TYPE_NAVIGATION_PAGE)

enum {
    PROP_0,
    PROP_MANAGER,
    PROP_SHELL_VERSION,
    PROP_DATA,
    PROP_EXTENSION,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmDetailView *
exm_detail_view_new (void)
{
    return g_object_new (EXM_TYPE_DETAIL_VIEW, NULL);
}

static void
exm_detail_view_finalize (GObject *object)
{
    ExmDetailView *self = (ExmDetailView *)object;

    G_OBJECT_CLASS (exm_detail_view_parent_class)->finalize (object);
}

static void
exm_detail_view_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
    ExmDetailView *self = EXM_DETAIL_VIEW (object);

    switch (prop_id)
    {
    case PROP_MANAGER:
        g_value_set_object (value, self->manager);
        break;
    case PROP_SHELL_VERSION:
        g_value_set_string (value, self->shell_version);
        break;
    case PROP_DATA:
        g_value_set_object (value, self->data);
        break;
    case PROP_EXTENSION:
        g_value_set_object (value, self->extension);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void on_bind_manager (ExmDetailView *self);

static void
exm_detail_view_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
    ExmDetailView *self = EXM_DETAIL_VIEW (object);

    switch (prop_id)
    {
    case PROP_MANAGER:
        self->manager = g_value_get_object (value);
        on_bind_manager (self);
        break;
    case PROP_SHELL_VERSION:
        self->shell_version = g_value_dup_string (value);
        break;
    case PROP_DATA:
        self->data = g_value_get_object (value);
        break;
    case PROP_EXTENSION:
        self->extension = g_value_get_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
on_icon_loaded (GObject       *source,
                GAsyncResult  *res,
                ExmDetailView *self)
{
    GError *error = NULL;
    GdkTexture *texture = exm_image_resolver_resolve_finish (EXM_IMAGE_RESOLVER (source),
                                                             res, &error);

    if (error)
    {
        // TODO: Properly log this
        g_critical ("%s\n", error->message);
        return;
    }

    gtk_image_set_from_paintable (self->ext_icon, GDK_PAINTABLE (texture));
    g_object_unref (texture);
    g_object_unref (self);
}

static void
on_image_loaded (GObject       *source,
                 GAsyncResult  *res,
                 ExmDetailView *self)
{
    GError *error = NULL;
    GdkTexture *texture = exm_image_resolver_resolve_finish (EXM_IMAGE_RESOLVER (source),
                                                             res, &error);
    AdwNavigationView *parent;
    ExmScreenshotView *screenshot_view;

    if (error)
    {
        // TODO: Properly log this
        g_critical ("%s\n", error->message);
        return;
    }

    parent = ADW_NAVIGATION_VIEW (gtk_widget_get_parent (GTK_WIDGET (self)));
    screenshot_view = EXM_SCREENSHOT_VIEW (adw_navigation_view_find_page (parent, "screenshot-view"));

    exm_screenshot_set_paintable (self->ext_screenshot, GDK_PAINTABLE (texture));
    exm_screenshot_view_set_screenshot (screenshot_view, GDK_PAINTABLE (texture));
    exm_screenshot_display (self->ext_screenshot);
    g_object_unref (texture);
    g_object_unref (self);

	gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot_popout_button), TRUE);
}

static void
queue_resolve_image (ExmDetailView    *self,
                     const gchar      *image_uri,
                     GCancellable     *cancellable,
                     gboolean          is_icon)
{
    exm_image_resolver_resolve_async (self->resolver, image_uri, cancellable,
                                      (GAsyncReadyCallback) (is_icon ? on_icon_loaded : on_image_loaded),
                                      g_object_ref (self));
}

static GtkWidget *
comment_factory (ExmComment *comment)
{
    GtkWidget *tile;

    tile = gtk_flow_box_child_new ();
    gtk_widget_add_css_class (tile, "card");
    gtk_flow_box_child_set_child (GTK_FLOW_BOX_CHILD (tile), GTK_WIDGET (exm_comment_tile_new (comment)));

    return tile;
}

static void
on_get_comments (GObject       *source,
                 GAsyncResult  *res,
                 ExmDetailView *self)
{
    GError *error = NULL;

    GListModel *model = exm_comment_provider_get_comments_finish (EXM_COMMENT_PROVIDER (source), res, &error);

    if (error != NULL)
    {
        gtk_stack_set_visible_child_name (self->comment_stack, "page_error");
        g_critical ("An issue occurred while loading comments: %s", error->message);
        return;
    }

    if (g_list_model_get_n_items (model) == 0)
    {
        gtk_stack_set_visible_child_name (self->comment_stack, "page_empty");
    }
    else
    {
        gtk_stack_set_visible_child_name (self->comment_stack, "page_comments");
    }

    gtk_flow_box_bind_model (self->comment_box, model,
                             (GtkListBoxCreateWidgetFunc) comment_factory,
                             g_object_ref (self), g_object_unref);
}

static void
queue_resolve_comments (ExmDetailView *self,
                        gint           pk,
                        GCancellable  *cancellable)
{
    gtk_stack_set_visible_child_name (self->comment_stack, "page_spinner");
    exm_comment_provider_get_comments_async (self->comment_provider, pk, false, cancellable,
                                             (GAsyncReadyCallback) on_get_comments,
                                             self);
}

static void
show_more_comments (GtkButton *button,
                    ExmDetailView *self)
{
    GtkRoot *toplevel;
    ExmCommentDialog *dlg;

    dlg = exm_comment_dialog_new (self->pk);
    toplevel = gtk_widget_get_root (GTK_WIDGET (self));

    adw_dialog_present (ADW_DIALOG (dlg), GTK_WIDGET (toplevel));
}

static void
install_remote (GtkButton     *button,
                ExmDetailView *self)
{
    gboolean warn;
    ExmInstallButtonState state;

    g_object_get (self->ext_install, "state", &state, NULL);

    warn = (state == EXM_INSTALL_BUTTON_STATE_UNSUPPORTED);
    gtk_widget_activate_action (GTK_WIDGET (button),
                                "ext.install",
                                "(sb)", self->uuid, warn);
}

static void
open_prefs (GSimpleAction *action,
            GVariant      *new_value,
            ExmDetailView *self)
{
    g_return_if_fail (self->extension);

    gtk_widget_activate_action (GTK_WIDGET (self),
                                "ext.open-prefs",
                                "s", self->uuid);
}

static void
uninstall (GSimpleAction *action,
           GVariant      *new_value,
           ExmDetailView *self)
{
    g_return_if_fail (self->extension);

    gtk_widget_activate_action (GTK_WIDGET (self),
                                "ext.remove",
                                "s", self->uuid);
}

static gboolean
transform_to_state (GBinding     *binding,
                    const GValue *from_value,
                    GValue       *to_value,
                    gpointer      user_data)
{
    g_value_set_boolean (to_value, g_value_get_enum (from_value) == EXM_EXTENSION_STATE_ACTIVE);

    return TRUE;
}

static gboolean
on_state_changed (GtkSwitch     *toggle,
                  gboolean       state,
                  ExmDetailView *self)
{
    g_return_val_if_fail (EXM_IS_DETAIL_VIEW (self), FALSE);

    gboolean enabled;

    g_object_get (self->extension, "enabled", &enabled, NULL);

    // Prevents changing extensions' state when global switch is toggled
    if (state == enabled)
        return TRUE;

    // Keep compatibility with GNOME Shell versions prior to 46
    if (gtk_switch_get_state (toggle) != enabled)
        g_object_set (self->extension, "enabled", !enabled, NULL);

    if (state)
        exm_manager_enable_extension (self->manager, self->extension);
    else
        exm_manager_disable_extension (self->manager, self->extension);

    return TRUE;
}

static void
populate_with_data (ExmUnifiedData *data,
                    gpointer      user_data)
{
    // TODO: Slowly migrate all of this over to Blueprint
    GError *error = NULL;
    ExmDetailView *self;
    ExmInstallButtonState install_state;
    GList *version_iter;
    ExmShellVersionMap *version_map;
    int downloads;
    char *screenshot_uri;

    self = EXM_DETAIL_VIEW (user_data);

    gboolean is_installed, is_supported;

    is_installed = exm_manager_is_installed_uuid (self->manager, exm_unified_data_get_uuid (data));
    is_supported = exm_shell_version_map_supports (exm_unified_data_get_shell_version_map (data), self->shell_version);

    gtk_image_set_from_icon_name (self->ext_icon, "puzzle-piece-symbolic");
    exm_unified_data_get_downloads (data, &downloads);
    exm_info_bar_set_downloads (self->ext_info_bar, downloads);

    if (self->resolver_cancel)
    {
        g_cancellable_cancel (self->resolver_cancel);
        g_clear_object (&self->resolver_cancel);
    }

    char *icon_uri;
    exm_unified_data_get_icon_uri (data, &icon_uri);
    if (strcmp (icon_uri, "/static/images/plugin.png") != 0)
    {
        self->resolver_cancel = g_cancellable_new ();

        queue_resolve_image (self, icon_uri, self->resolver_cancel, TRUE);
    }

    if (exm_unified_data_get_screenshot_uri (data, &screenshot_uri))
    {
        self->resolver_cancel = g_cancellable_new ();

        exm_screenshot_set_paintable (self->ext_screenshot, NULL);
        exm_screenshot_reset (self->ext_screenshot);

	    gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot_container), TRUE);
	    gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot_popout_button), FALSE);

        queue_resolve_image (self, screenshot_uri, self->resolver_cancel, FALSE);
    }
    else
    {
        gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot_container), FALSE);
    }

    install_state = is_installed
        ? EXM_INSTALL_BUTTON_STATE_INSTALLED
        : (is_supported
           ? EXM_INSTALL_BUTTON_STATE_DEFAULT
           : EXM_INSTALL_BUTTON_STATE_UNSUPPORTED);

    if (is_installed)
    {
        gtk_stack_set_visible_child_name (self->tools_stack, "installed-tools-page");
    }
    else
    {
        gtk_stack_set_visible_child_name (self->tools_stack, "uninstalled-tools-page");
    }

    g_object_set (self->ext_install, "state", install_state, NULL);

    char *url;
    exm_unified_data_get_homepage (data, &url);
    self->uri_homepage = g_uri_resolve_relative (url,
                                                 "",
                                                 G_URI_FLAGS_NONE,
                                                 NULL);

    char *link;
    exm_unified_data_get_link (data, &link);
    self->uri_extensions = g_uri_resolve_relative ("https://extensions.gnome.org/",
                                                   link,
                                                   G_URI_FLAGS_NONE,
                                                   NULL);

    adw_action_row_set_subtitle (self->link_homepage, self->uri_homepage);
    adw_action_row_set_subtitle (self->link_extensions, self->uri_extensions);

    exm_info_bar_set_version (self->ext_info_bar, -1);

    version_map = exm_unified_data_get_shell_version_map (data);

    for (version_iter = version_map->map;
         version_iter != NULL;
         version_iter = version_iter->next)
    {
        gchar *version;
        MapEntry *entry;

        entry = version_iter->data;

        if (entry->shell_minor_version)
            version = g_strdup_printf ("%s.%s", entry->shell_major_version, entry->shell_minor_version);
        else
            version = g_strdup_printf ("%s.0", entry->shell_major_version);

        if (strcmp (version, self->shell_version) == 0 || strncmp(version, self->shell_version, strchr(version, '.') - version) == 0)
            exm_info_bar_set_version (self->ext_info_bar, entry->extension_version);

        g_free (version);
    }

    exm_unified_data_get_pk (data, &self->pk);

    if (self->signal_id > 0)
        g_signal_handler_disconnect (self->show_more_btn, self->signal_id);

    self->signal_id = g_signal_connect (self->show_more_btn,
                                        "clicked",
                                        G_CALLBACK (show_more_comments),
                                        self);

    queue_resolve_comments (self, self->pk, self->resolver_cancel);
}

static void
on_data_loaded (GObject      *source,
                GAsyncResult *async_result,
                gpointer      user_data)
{
    ExmUnifiedData *data;
    ExmSearchResult *web_info;
    ExmExtension *local_info;

    GError *error = NULL;
    ExmDetailView *self;

    g_return_if_fail (EXM_IS_DETAIL_VIEW (user_data));

    self = EXM_DETAIL_VIEW (user_data);

    g_object_get (G_OBJECT (self),
                  "data", &data,
                  "extension", &local_info,
                  NULL);

    if ((web_info = exm_data_provider_get_finish (EXM_DATA_PROVIDER (source), async_result, &error)) != FALSE)
        exm_unified_data_set_web_data (data, web_info);

    // We need at least some data to proceed
    if (!exm_unified_data_is_empty (data)) {
        populate_with_data (data, self);

        gchar *name, *uuid, *description, *version, *error_msg;
        gboolean enabled, has_prefs, has_update, is_user;
        ExmExtensionState state;
        g_object_get (self->extension,
                      "display-name", &name,
                      "uuid", &uuid,
                      "description", &description,
                      "state", &state,
                      "enabled", &enabled,
                      "has-prefs", &has_prefs,
                      "has-update", &has_update,
                      "is-user", &is_user,
                      "version", &version,
                      "error-msg", &error_msg,
                      NULL);

        GAction *action;

        action = g_action_map_lookup_action (G_ACTION_MAP (self->action_group), "open-prefs");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), has_prefs);

        action = g_action_map_lookup_action (G_ACTION_MAP (self->action_group), "remove");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), is_user);

        g_object_bind_property_full (self->extension,
                                     "state",
                                     self->ext_toggle,
                                     "state",
                                     G_BINDING_SYNC_CREATE,
                                     transform_to_state,
                                     NULL,
                                     NULL,
                                     NULL);

        // Reset scroll position
        gtk_adjustment_set_value (gtk_scrolled_window_get_vadjustment (self->scroll_area), 0);
        gtk_stack_set_visible_child_name (self->stack, "page_detail");

        return;
    }

    adw_window_title_set_title (self->title, _("An Error Occurred"));
    adw_window_title_set_subtitle (self->title, NULL);
    gtk_stack_set_visible_child_name (self->stack, "page_error");
    return;
}

void
exm_detail_view_load_for_uuid (ExmDetailView *self,
                               gchar         *uuid)
{
    ExmUnifiedData *data;
    ExmExtension *local_info;

    self->uuid = uuid;

    adw_window_title_set_title (self->title, NULL);
    adw_window_title_set_subtitle (self->title, NULL);

    // gtk_stack_set_visible_child_name (self->stack, "page_spinner");
    gtk_stack_set_visible_child_name (self->stack, "page_detail");

    data = exm_unified_data_new ();

    // Build Unified Data Representation
    if ((local_info = exm_manager_get_by_uuid (self->manager, self->uuid)))
        exm_unified_data_set_local_data (data, local_info);

    g_object_set (self,
                  "data", data,
                  "extension", local_info,
                  NULL);

    exm_data_provider_get_async (self->provider, uuid, NULL, on_data_loaded, self);
}

void
exm_detail_view_update (ExmDetailView *self)
{
    if (!self->uuid)
        return;

    // Check if the newly installed extension is the
    // one being displayed in this detail view
    if (exm_manager_is_installed_uuid (self->manager, self->uuid))
    {
        g_object_set (self->ext_install, "state", EXM_INSTALL_BUTTON_STATE_INSTALLED, NULL);
    }
}

static void
open_link (ExmDetailView *self,
           const char    *action_name,
           GVariant      *param)
{
    GtkWidget *toplevel;
    GtkUriLauncher *uri = NULL;

    toplevel = GTK_WIDGET (gtk_widget_get_root (GTK_WIDGET (self)));

    if (strcmp (action_name, "detail.open-extensions") == 0)
        uri = gtk_uri_launcher_new (self->uri_extensions);
    else if (strcmp (action_name, "detail.open-homepage") == 0)
        uri = gtk_uri_launcher_new (self->uri_homepage);
    else
        g_critical ("open_link() invalid action: %s", action_name);

    gtk_uri_launcher_launch (uri, GTK_WINDOW (toplevel), NULL, NULL, NULL);
}

static void
on_bind_manager (ExmDetailView *self)
{
    GListModel *user_ext_model;
    GListModel *system_ext_model;

    g_object_get (self->manager,
                  "user-extensions", &user_ext_model,
                  "system-extensions", &system_ext_model,
                  NULL);

    g_signal_connect_swapped (user_ext_model,
                              "items-changed",
                              G_CALLBACK (exm_detail_view_update),
                              self);

    g_signal_connect_swapped (system_ext_model,
                              "items-changed",
                              G_CALLBACK (exm_detail_view_update),
                              self);
}

static void
breakpoint_apply_cb (ExmDetailView *self)
{
    gtk_widget_remove_css_class (GTK_WIDGET (self->ext_title), "title-1");
    gtk_widget_add_css_class (GTK_WIDGET (self->ext_title), "title-2");
}

static void
breakpoint_unapply_cb (ExmDetailView *self)
{
    gtk_widget_remove_css_class (GTK_WIDGET (self->ext_title), "title-2");
    gtk_widget_add_css_class (GTK_WIDGET (self->ext_title), "title-1");
}

static void
update_headerbar_cb (ExmDetailView *self)
{
    GtkAdjustment *adj;

    adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (self->scroll_area));

    adw_header_bar_set_show_title (ADW_HEADER_BAR (self->header_bar),
                                   gtk_adjustment_get_value (adj) > 0);
}

static void
screenshot_view_cb (ExmDetailView *self)
{
    AdwNavigationView *parent;

    parent = ADW_NAVIGATION_VIEW (gtk_widget_get_parent (GTK_WIDGET (self)));
    adw_navigation_view_push_by_tag (parent, "screenshot-view");
}

static void
exm_detail_view_class_init (ExmDetailViewClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_detail_view_finalize;
    object_class->get_property = exm_detail_view_get_property;
    object_class->set_property = exm_detail_view_set_property;

    properties [PROP_MANAGER]
        = g_param_spec_object ("manager",
                               "Manager",
                               "Manager",
                               EXM_TYPE_MANAGER,
                               G_PARAM_READWRITE);

    properties [PROP_SHELL_VERSION]
        = g_param_spec_string ("shell-version",
                               "Shell Version",
                               "Shell Version",
                               NULL,
                               G_PARAM_READWRITE);

    properties [PROP_DATA]
        = g_param_spec_object ("data",
                               "Data",
                               "Data",
                               EXM_TYPE_UNIFIED_DATA,
                               G_PARAM_READWRITE);

    properties [PROP_EXTENSION]
        = g_param_spec_object ("extension",
                               "Extension",
                               "Extension",
                               EXM_TYPE_EXTENSION,
                               G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-detail-view.ui");

    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, header_bar);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, title);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, stack);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_title);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_author);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_description);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_install);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_screenshot);
	gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_screenshot_container);
	gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_screenshot_popout_button);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_info_bar);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, link_homepage);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, link_extensions);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, scroll_area);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, comment_box);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, comment_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, show_more_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, tools_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_toggle);

    gtk_widget_class_bind_template_callback (widget_class, breakpoint_apply_cb);
    gtk_widget_class_bind_template_callback (widget_class, breakpoint_unapply_cb);
    gtk_widget_class_bind_template_callback (widget_class, screenshot_view_cb);
    gtk_widget_class_bind_template_callback (widget_class, on_state_changed);

    gtk_widget_class_install_action (widget_class, "detail.open-extensions", NULL, open_link);
    gtk_widget_class_install_action (widget_class, "detail.open-homepage", NULL, open_link);
}

static void
exm_detail_view_init (ExmDetailView *self)
{
    GtkAdjustment *adj;

    gtk_widget_init_template (GTK_WIDGET (self));

    self->provider = exm_data_provider_new ();
    self->resolver = exm_image_resolver_new ();
    self->comment_provider = exm_comment_provider_new ();

    g_signal_connect (self->ext_install,
                      "clicked",
                      G_CALLBACK (install_remote),
                      self);

    adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (self->scroll_area));

    g_signal_connect_swapped (adj, "value-changed", G_CALLBACK (update_headerbar_cb), self);

    update_headerbar_cb (self);

    // Define Actions
    self->action_group = g_simple_action_group_new ();

    GSimpleAction *open_prefs_action = g_simple_action_new ("open-prefs", NULL);
    g_signal_connect (open_prefs_action, "activate", G_CALLBACK (open_prefs), self);

    GSimpleAction *remove_action = g_simple_action_new ("remove", NULL);
    g_signal_connect (remove_action, "activate", G_CALLBACK (uninstall), self);

    g_action_map_add_action (G_ACTION_MAP (self->action_group), G_ACTION (open_prefs_action));
    g_action_map_add_action (G_ACTION_MAP (self->action_group), G_ACTION (remove_action));

    gtk_widget_insert_action_group (GTK_WIDGET (self), "page", G_ACTION_GROUP (self->action_group));
}

