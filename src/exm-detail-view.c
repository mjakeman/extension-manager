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
#include "exm-zoom-picture.h"
#include "exm-comment-tile.h"
#include "exm-comment-dialog.h"

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
    GtkBox parent_instance;

    ExmManager *manager;
    ExmDataProvider *provider;
    ExmImageResolver *resolver;
    ExmCommentProvider *comment_provider;
    GCancellable *resolver_cancel;

  	GSimpleAction *zoom_in;
    GSimpleAction *zoom_out;

    gchar *shell_version;
    gchar *uuid;

    AdwWindowTitle *title;
    GtkStack *stack;
    GtkButton *ext_install;
    GtkLabel *ext_description;
    GtkLabel *ext_title;
    GtkLabel *ext_author;
    ExmScreenshot *ext_screenshot;
	GtkOverlay *ext_screenshot_container;
	GtkButton *ext_screenshot_popout_button;
	GtkButton *ext_screenshot_popin_button;
    GtkFlowBox *supported_versions;
    GtkScrolledWindow *scroll_area;
    GtkStack *comment_stack;
    GtkFlowBox *comment_box;
    GtkButton *show_more_btn;
	AdwBin *image_overlay;
	ExmZoomPicture *overlay_screenshot;

    AdwActionRow *link_extensions;
    gchar *uri_extensions;
    int pk;
    guint signal_id;
};

G_DEFINE_FINAL_TYPE (ExmDetailView, exm_detail_view, GTK_TYPE_BOX)

enum {
    PROP_0,
    PROP_MANAGER,
    PROP_SHELL_VERSION,
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
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
on_image_loaded (GObject       *source,
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

    exm_screenshot_set_paintable (self->ext_screenshot, GDK_PAINTABLE (texture));
	exm_zoom_picture_set_paintable (self->overlay_screenshot, GDK_PAINTABLE (texture));
    exm_screenshot_display (self->ext_screenshot);
	exm_zoom_picture_set_zoom_level (self->overlay_screenshot, 1.0f);
    g_object_unref (texture);
    g_object_unref (self);

	gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot_popout_button), TRUE);
}

static void
queue_resolve_screenshot (ExmDetailView    *self,
                          const gchar      *screenshot_uri,
                          GCancellable     *cancellable)
{
    exm_image_resolver_resolve_async (self->resolver, screenshot_uri, cancellable,
                                      (GAsyncReadyCallback) on_image_loaded,
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

    gtk_stack_set_visible_child_name (self->comment_stack, "page_comments");

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

    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (toplevel));
    gtk_window_set_modal (GTK_WINDOW (dlg), TRUE);

    gtk_window_present (GTK_WINDOW (dlg));
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
on_data_loaded (GObject      *source,
                GAsyncResult *result,
                gpointer      user_data)
{
    ExmSearchResult *data;
    GError *error = NULL;
    ExmDetailView *self;
    ExmInstallButtonState install_state;
    GtkWidget *child;
    GList *version_iter;
    ExmShellVersionMap *version_map;
    gchar *uri;

    self = EXM_DETAIL_VIEW (user_data);

    if ((data = exm_data_provider_get_finish (EXM_DATA_PROVIDER (source), result, &error)) != FALSE)
    {
        gint pk;
        gboolean is_installed, is_supported;
        gchar *uuid, *name, *creator, *icon_uri, *screenshot_uri, *link, *description;
        g_object_get (data,
                      "uuid", &uuid,
                      "name", &name,
                      "creator", &creator,
                      "icon", &icon_uri,
                      "screenshot", &screenshot_uri,
                      "link", &link,
                      "description", &description,
                      "shell_version_map", &version_map,
                      "pk", &pk,
                      NULL);

        adw_window_title_set_title (self->title, name);
        adw_window_title_set_subtitle (self->title, uuid);

        is_installed = exm_manager_is_installed_uuid (self->manager, uuid);
        is_supported = exm_search_result_supports_shell_version (data, self->shell_version);

        gtk_label_set_label (self->ext_title, name);
        gtk_label_set_label (self->ext_author, creator);
        gtk_label_set_label (self->ext_description, description);

        if (self->resolver_cancel)
        {
            g_cancellable_cancel (self->resolver_cancel);
            g_clear_object (&self->resolver_cancel);
        }

        if (screenshot_uri != NULL)
        {
            self->resolver_cancel = g_cancellable_new ();

            exm_screenshot_set_paintable (self->ext_screenshot, NULL);
			exm_zoom_picture_set_paintable (self->overlay_screenshot, NULL);
            exm_screenshot_reset (self->ext_screenshot);

			gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot_container), TRUE);
			gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot_popout_button), FALSE);

            queue_resolve_screenshot (self, screenshot_uri, self->resolver_cancel);
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

        g_object_set (self->ext_install, "state", install_state, NULL);

        self->uri_extensions = g_uri_resolve_relative ("https://extensions.gnome.org/",
                                                       link,
                                                       G_URI_FLAGS_NONE,
                                                       NULL);

        adw_action_row_set_subtitle (self->link_extensions, self->uri_extensions);

        // Clear Flowbox
        while ((child = gtk_widget_get_first_child (GTK_WIDGET (self->supported_versions))))
            gtk_flow_box_remove (self->supported_versions, child);

        for (version_iter = version_map->map;
             version_iter != NULL;
             version_iter = version_iter->next)
        {
            gchar *version;
            MapEntry *entry;
            GtkWidget *label;

            entry = version_iter->data;

            if (entry->shell_minor_version)
                version = g_strdup_printf ("%s.%s", entry->shell_major_version, entry->shell_minor_version);
            else
                version = g_strdup (entry->shell_major_version);

            label = gtk_label_new (version);
            gtk_widget_add_css_class (label, "version-label");
            gtk_flow_box_prepend (self->supported_versions, label);

            g_free (version);
        }

        self->pk = pk;

        if (self->signal_id > 0)
            g_signal_handler_disconnect (self->show_more_btn, self->signal_id);

        self->signal_id = g_signal_connect (self->show_more_btn,
                                            "clicked",
                                            G_CALLBACK (show_more_comments),
                                            self);

        queue_resolve_comments (self, pk, self->resolver_cancel);

        // Reset scroll position
        gtk_adjustment_set_value (gtk_scrolled_window_get_vadjustment (self->scroll_area), 0);

        gtk_stack_set_visible_child_name (self->stack, "page_detail");

        return;
    }

    adw_window_title_set_title (self->title, _("An Error Occurred"));
    adw_window_title_set_subtitle (self->title, NULL);
    gtk_stack_set_visible_child_name (self->stack, "page_error");
}

void
exm_detail_view_load_for_uuid (ExmDetailView *self,
                               gchar         *uuid)
{
    // g_assert (gtk_widget_is_constructed)

    self->uuid = uuid;

    /* Translators: Use unicode ellipsis '…' rather than three dots '...' */
    adw_window_title_set_title (self->title, _("Loading…"));
    adw_window_title_set_subtitle (self->title, NULL);

    gtk_stack_set_visible_child_name (self->stack, "page_spinner");
	gtk_widget_hide (GTK_WIDGET (self->image_overlay));

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
    gchar *uri = NULL;

    toplevel = GTK_WIDGET (gtk_widget_get_root (GTK_WIDGET (self)));

    if (strcmp (action_name, "detail.open-extensions") == 0)
        uri = self->uri_extensions;
    else if (strcmp (action_name, "detail.open-homepage") == 0)
        g_warning ("open_link(): cannot open homepage as not yet implemented.");
    else
        g_critical ("open_link() invalid action: %s", action_name);

    gtk_show_uri (GTK_WINDOW (toplevel), uri, GDK_CURRENT_TIME);
}

static void
notify_zoom (ExmZoomPicture *picture,
             GParamSpec     *pspec,
             ExmDetailView  *self)
{
    float zoom_level;
    float max_zoom;
    float min_zoom;

    zoom_level = exm_zoom_picture_get_zoom_level (picture);
    max_zoom = exm_zoom_picture_get_zoom_level_max (picture);
    min_zoom = exm_zoom_picture_get_zoom_level_min (picture);

	// Set action states
	if (zoom_level < max_zoom)
		g_simple_action_set_enabled (self->zoom_in, TRUE);
	if (zoom_level == max_zoom)
		g_simple_action_set_enabled (self->zoom_in, FALSE);
	if (zoom_level > min_zoom)
		g_simple_action_set_enabled (self->zoom_out, TRUE);
	if (zoom_level == min_zoom)
		g_simple_action_set_enabled (self->zoom_out, FALSE);
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

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-detail-view.ui");

    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, title);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, stack);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_title);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_author);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_description);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_install);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_screenshot);
	gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_screenshot_container);
	gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_screenshot_popout_button);
	gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_screenshot_popin_button);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, supported_versions);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, link_extensions);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, scroll_area);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, comment_box);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, comment_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, show_more_btn);
	gtk_widget_class_bind_template_child (widget_class, ExmDetailView, image_overlay);
	gtk_widget_class_bind_template_child (widget_class, ExmDetailView, overlay_screenshot);

    gtk_widget_class_install_action (widget_class, "detail.open-extensions", NULL, open_link);
    gtk_widget_class_install_action (widget_class, "detail.open-homepage", NULL, open_link);
}

static void
exm_detail_view_init (ExmDetailView *self)
{
    GSimpleActionGroup *group;

    gtk_widget_init_template (GTK_WIDGET (self));

    self->provider = exm_data_provider_new ();
    self->resolver = exm_image_resolver_new ();
    self->comment_provider = exm_comment_provider_new ();

	self->zoom_in = g_simple_action_new ("zoom-in", NULL);
	g_signal_connect_swapped (self->zoom_in, "activate", G_CALLBACK (exm_zoom_picture_zoom_in), self->overlay_screenshot);

	self->zoom_out = g_simple_action_new ("zoom-out", NULL);
	g_signal_connect_swapped (self->zoom_out, "activate", G_CALLBACK (exm_zoom_picture_zoom_out), self->overlay_screenshot);

	group = g_simple_action_group_new ();
	g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (self->zoom_in));
	g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (self->zoom_out));
	gtk_widget_insert_action_group (GTK_WIDGET (self), "detail", G_ACTION_GROUP (group));

    // Update action state on zoom change
    g_signal_connect (self->overlay_screenshot,
                      "notify::zoom-level",
                      G_CALLBACK (notify_zoom),
                      self);

    g_signal_connect (self->ext_install,
                      "clicked",
                      G_CALLBACK (install_remote),
                      self);

	g_signal_connect_swapped (self->ext_screenshot_popout_button,
							  "clicked",
							  G_CALLBACK (gtk_widget_show),
							  self->image_overlay);

	g_signal_connect_swapped (self->ext_screenshot_popin_button,
							  "clicked",
							  G_CALLBACK (gtk_widget_hide),
							  self->image_overlay);
}
