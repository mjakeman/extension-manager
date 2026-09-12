/*
 * exm-detail-view.c
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

#include "exm-comment-dialog.h"
#include "exm-comment-tile.h"
#include "exm-config.h"
#include "exm-enums.h"
#include "exm-extension-error-dialog.h"
#include "exm-extension-toggle.h"
#include "exm-install-button.h"
#include "exm-screenshot.h"
#include "exm-screenshot-view.h"
#include "exm-types.h"
#include "exm-versions-dialog.h"
#include "exm-unified-data.h"
#include "local/exm-manager.h"
#include "web/exm-comment-provider.h"
#include "web/exm-data-provider.h"
#include "web/exm-image-resolver.h"
#include "web/exm-versions-provider.h"
#include "web/model/exm-comment.h"
#include "web/model/exm-shell-versions.h"
#include "web/model/exm-version-result.h"

#include <glib/gi18n.h>

struct _ExmDetailView
{
    AdwNavigationPage parent_instance;

    ExmUnifiedData *data;

    ExmManager *manager;
    ExmDataProvider *provider;
    ExmImageResolver *resolver;
    ExmVersionsProvider *versions_provider;
    ExmCommentProvider *comment_provider;
    GCancellable *resolver_cancel;

    GdkPaintable *screenshot;
    gchar *shell_version;
    gchar *uuid;

    AdwHeaderBar *header_bar;
    GtkRevealer *title_revealer;
    AdwToastOverlay *toast_overlay;
    GtkStack *stack;
    GtkLabel *error_label;
    AdwStatusPage *error_status;
    GtkStack *tools_stack;
    GtkButton *ext_install;
    GtkButton *remove_btn;
    GtkButton *prefs_btn;
    GtkSwitch *ext_toggle;
    GtkLabel *ext_description;
    GtkImage *ext_icon;
    GtkLabel *ext_title;
    GtkLabel *ext_author;
    ExmScreenshot *ext_screenshot;
    GtkOverlay *ext_screenshot_container;
    GtkButton *ext_screenshot_popout_button;
    AdwActionRow *downloads_row;
    AdwActionRow *version_row;
    GtkImage *version_row_go_next;
    GtkImage *update_icon;
    GtkImage *out_of_date_icon;
    GtkLabel *version_label;
    AdwActionRow *session_modes_row;
    GtkLabel *session_modes_label;
    AdwActionRow *location_row;
    GtkImage *location_row_folder_icon;
    GtkImage *location_row_copy_icon;
    AdwBanner *error_banner;
    gchar *error_text;
    AdwPreferencesGroup *links_group;
    GtkWidget *comments_section;
    ExmVersionsDialog *ext_versions_dialog;
    gchar             *versions_next_url;
    GtkScrolledWindow *scroll_area;
    GtkStack *comment_stack;
    AdwWrapBox *comment_box;
    GtkButton *show_more_btn;

    GSimpleActionGroup *action_group;

    AdwActionRow *link_homepage;
    AdwActionRow *link_donation;
    AdwExpanderRow *links_donations;
    gchar **uri_donations;
    GList *donation_rows_list;
    AdwActionRow *link_extensions;
    gchar *uri_extensions;
    guint id;
    guint comments_signal_id;
    guint install_signal_id;
    ExmInstallButtonState version_state;
};

G_DEFINE_FINAL_TYPE (ExmDetailView, exm_detail_view, ADW_TYPE_NAVIGATION_PAGE)

enum {
    PROP_0,
    PROP_MANAGER,
    PROP_SHELL_VERSION,
    PROP_SCREENSHOT,
    PROP_DATA,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmDetailView *
exm_detail_view_new (void)
{
    return g_object_new (EXM_TYPE_DETAIL_VIEW, NULL);
}

static void
exm_detail_view_dispose (GObject *object)
{
    ExmDetailView *self = EXM_DETAIL_VIEW (object);

    g_clear_object (&self->data);
    g_clear_object (&self->action_group);
    g_clear_pointer (&self->uuid, g_free);
    g_clear_pointer (&self->error_text, g_free);

    G_OBJECT_CLASS (exm_detail_view_parent_class)->dispose (object);
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
    case PROP_SCREENSHOT:
        g_value_set_object (value, self->screenshot);
        break;
    case PROP_DATA:
        g_value_set_object (value, self->data);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void on_bind_manager    (ExmDetailView *self);
static void update_tools_stack (ExmDetailView *self);

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
    case PROP_SCREENSHOT:
        self->screenshot = g_value_get_object (value);
        break;
    case PROP_DATA:
        g_set_object (&self->data, g_value_get_object (value));
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
    GdkPaintable *paintable = exm_image_resolver_resolve_finish (EXM_IMAGE_RESOLVER (source),
                                                                  res, &error);

    if (error)
    {
        // TODO: Properly log this
        g_critical ("%s\n", error->message);
        return;
    }

    gtk_image_set_from_paintable (self->ext_icon, paintable);
    g_object_unref (paintable);
    g_object_unref (self);
}

static void
on_image_loaded (GObject       *source,
                 GAsyncResult  *res,
                 ExmDetailView *self)
{
    GError *error = NULL;
    GdkPaintable *paintable = exm_image_resolver_resolve_finish (EXM_IMAGE_RESOLVER (source),
                                                                  res, &error);

    if (error)
    {
        // TODO: Properly log this
        g_critical ("%s\n", error->message);
        return;
    }

    g_object_set (self, "screenshot", paintable, NULL);
    exm_screenshot_set_paintable (self->ext_screenshot, paintable);
    exm_screenshot_display (self->ext_screenshot);
    g_object_unref (paintable);
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

static void
delete_comment_tiles (ExmDetailView *self)
{
  GtkWidget *child;

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self->comment_box))) != NULL)
    adw_wrap_box_remove (self->comment_box, child);
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
        if (error->domain == g_quark_try_string ("request-error-quark") && error->code == 403)
        {
            gtk_stack_set_visible_child_name (self->comment_stack, "page_disabled");
        }
        else
        {
            gtk_stack_set_visible_child_name (self->comment_stack, "page_error");
            gtk_label_set_label (self->error_label, error->message);
        }

        return;
    }

    guint n_comments = g_list_model_get_n_items (model);

    if (!n_comments)
    {
        gtk_stack_set_visible_child_name (self->comment_stack, "page_empty");
    }
    else
    {
        gtk_widget_set_visible (GTK_WIDGET (self->show_more_btn), n_comments >= 5);
        gtk_stack_set_visible_child_name (self->comment_stack, "page_comments");
    }

    delete_comment_tiles (self);

    for (guint i = 0; i < g_list_model_get_n_items (model); i++)
    {
        ExmComment *comment = g_list_model_get_item (model, i);
        GtkWidget *tile = GTK_WIDGET (exm_comment_tile_new (comment));
        gtk_widget_add_css_class (tile, "card");
        adw_wrap_box_append (self->comment_box, tile);
        g_object_unref (comment);
    }
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
show_more_comments (GtkButton     *button G_GNUC_UNUSED,
                    ExmDetailView *self)
{
    GtkRoot *toplevel;
    ExmCommentDialog *dlg;

    dlg = exm_comment_dialog_new (self->id);
    toplevel = gtk_widget_get_root (GTK_WIDGET (self));

    adw_dialog_present (ADW_DIALOG (dlg), GTK_WIDGET (toplevel));
}

static void
on_install_status (ExmManager            *manager G_GNUC_UNUSED,
                   ExmInstallButtonState  state,
                   ExmDetailView         *self)
{
    g_object_set (self->ext_install, "state", state, NULL);
}

static void
install_remote (GtkButton     *button,
                ExmDetailView *self)
{
    gboolean warn;
    ExmInstallButtonState state;

    g_object_get (self->ext_install, "state", &state, NULL);

    warn = (state == EXM_INSTALL_BUTTON_STATE_UNSUPPORTED);

    g_object_set (self->ext_install, "state", EXM_INSTALL_BUTTON_STATE_INSTALLING, NULL);

    gtk_widget_grab_focus (GTK_WIDGET (self->ext_author));

    gtk_widget_activate_action (GTK_WIDGET (button),
                                "ext.install",
                                "(sb)", self->uuid, warn);
}

static void
new_donation_row (ExmDetailView *self,
                  int            num_donation)
{
    GtkWidget *row;
    GtkWidget *external_link_icon;

    row = adw_action_row_new ();

    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), self->uri_donations[num_donation]);
    gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), TRUE);

    gtk_actionable_set_action_name (GTK_ACTIONABLE (row), "detail.open-donations");
    gtk_actionable_set_action_target_value (GTK_ACTIONABLE (row), g_variant_new_int32 (num_donation));

    external_link_icon = gtk_image_new_from_icon_name ("external-link-symbolic");
    adw_action_row_add_suffix (ADW_ACTION_ROW (row), external_link_icon);

    adw_expander_row_add_row (self->links_donations, row);

    self->donation_rows_list = g_list_append (self->donation_rows_list, row);
}

static void
delete_donation_rows (ExmDetailView *self)
{
    for (GList *iter = self->donation_rows_list; iter != NULL; iter = g_list_next (iter))
        adw_expander_row_remove (self->links_donations, GTK_WIDGET (iter->data));

    g_list_free (self->donation_rows_list);
    self->donation_rows_list = NULL;
}

static void
update_donation_rows (ExmDetailView  *self,
                      gchar         **donation_urls)
{
    guint n_urls;

    gtk_widget_set_visible (GTK_WIDGET (self->link_donation), FALSE);
    gtk_widget_set_visible (GTK_WIDGET (self->links_donations), FALSE);
    delete_donation_rows (self);

    if (donation_urls == NULL || donation_urls[0] == NULL)
        return;

    n_urls = g_strv_length (donation_urls);

    self->uri_donations = g_new0 (gchar *, n_urls);

    for (guint i = 0; donation_urls[i] != NULL; i++)
    {
        self->uri_donations[i] = g_uri_resolve_relative (donation_urls[i],
                                                         "",
                                                         G_URI_FLAGS_NONE,
                                                         NULL);
    }

    if (n_urls == 1)
    {
        adw_action_row_set_subtitle (self->link_donation, self->uri_donations[0]);
        gtk_actionable_set_action_name (GTK_ACTIONABLE (self->link_donation), "detail.open-donation");
        gtk_widget_set_visible (GTK_WIDGET (self->link_donation), TRUE);
    }
    else
    {
        for (guint i = 0; donation_urls[i] != NULL; i++)
            new_donation_row (self, i);

        adw_expander_row_set_expanded (self->links_donations, FALSE);
        gtk_widget_set_visible (GTK_WIDGET (self->links_donations), TRUE);
    }
}

static gchar *
format_downloads (GObject *object G_GNUC_UNUSED,
                  guint    downloads)
{
    return g_strdup_printf ("%'u", downloads);
}

static gboolean
has_homepage (GObject     *object G_GNUC_UNUSED,
              const gchar *homepage)
{
    return homepage != NULL && homepage[0] != '\0';
}

static gchar *
format_author_link (GObject     *object G_GNUC_UNUSED,
                    const gchar *creator,
                    const gchar *username)
{
    gchar *escaped;
    gchar *markup;

    if (creator == NULL || creator[0] == '\0')
        return g_strdup ("");

    escaped = g_markup_escape_text (creator, -1);

    if (username == NULL || username[0] == '\0')
        return escaped;

    markup = g_strdup_printf ("<a href=\"author\">%s</a>", escaped);
    g_free (escaped);

    return markup;
}

static gboolean
on_author_link_activated (GtkLabel      *label G_GNUC_UNUSED,
                          const gchar   *uri G_GNUC_UNUSED,
                          ExmDetailView *self)
{
    guint creator_id = 0;
    const gchar *username;
    const gchar *display_name;

    if (self->data == NULL)
        return GDK_EVENT_PROPAGATE;

    username = exm_unified_data_get_creator_username (self->data);

    if (username == NULL || username[0] == '\0')
        return GDK_EVENT_PROPAGATE;

    display_name = exm_unified_data_get_creator (self->data);
    exm_unified_data_get_creator_id (self->data, &creator_id);

    gtk_widget_grab_focus (GTK_WIDGET (self->ext_icon));

    gtk_widget_activate_action (GTK_WIDGET (self), "win.show-author", "(uss)",
                                creator_id, username, display_name);

    return GDK_EVENT_STOP;
}

static gchar *
format_session_modes (gchar **session_modes)
{
    static const struct { const char *key; const char *label; } known[] = {
        { "unlock-dialog", N_("Unlock Dialog") },
        { "gdm",           N_("GDM")           },
    };

    GPtrArray *parts = g_ptr_array_new ();

    for (gint i = 0; session_modes[i] != NULL; i++)
    {
        for (gsize j = 0; j < G_N_ELEMENTS (known); j++)
        {
            if (g_strcmp0 (session_modes[i], known[j].key) == 0)
            {
                g_ptr_array_add (parts, (gpointer) _(known[j].label));
                break;
            }
        }
    }

    gchar *result = NULL;
    if (parts->len > 0)
    {
        g_ptr_array_add (parts, NULL);
        result = g_strjoinv (", ", (gchar **) parts->pdata);
    }

    g_ptr_array_free (parts, FALSE);
    return result ? result : g_strdup ("");
}

static void
update_session_modes_row (ExmDetailView *self,
                          gchar        **session_modes)
{
    g_autofree gchar *label = (session_modes && session_modes[0])
                              ? format_session_modes (session_modes)
                              : NULL;
    gboolean visible = label && label[0];

    gtk_widget_set_visible (GTK_WIDGET (self->session_modes_row), visible);

    if (visible)
        gtk_label_set_label (self->session_modes_label, label);
}

static gboolean
update_session_modes_row_from_local (ExmDetailView *self)
{
    gchar **session_modes = NULL;
    gboolean found = exm_unified_data_get_session_modes (self->data, &session_modes);

    update_session_modes_row (self, session_modes);
    g_strfreev (session_modes);
    return found;
}

static void on_version_loaded (GObject *source, GAsyncResult *result, gpointer user_data);

static void
on_load_more_versions (ExmDetailView     *self,
                       ExmVersionsDialog *dialog G_GNUC_UNUSED)
{
    if (!self->versions_next_url)
        return;

    exm_versions_provider_query_next_async (self->versions_provider,
                                            self->versions_next_url,
                                            self->resolver_cancel,
                                            on_version_loaded,
                                            self);
}

static void
on_version_loaded (GObject      *source,
                   GAsyncResult *result,
                   gpointer      user_data)
{
    gchar *next;
    GError *error = NULL;
    GListModel *list;
    ExmDetailView *self;
    gboolean is_installed;
    ExmInstallButtonState install_state;
    ExmVersionResult *compatible_version = NULL;

    list = exm_versions_provider_query_finish (EXM_VERSIONS_PROVIDER (source), result, &next, &error);
    self = EXM_DETAIL_VIEW (user_data);

    is_installed = exm_manager_is_installed_uuid (self->manager, self->uuid);

    if (error)
    {
        install_state = is_installed ? EXM_INSTALL_BUTTON_STATE_INSTALLED
                                     : EXM_INSTALL_BUTTON_STATE_UNSUPPORTED;

        g_object_set (self->ext_install, "state", install_state, NULL);
        gtk_label_set_label (self->version_label, _("Unsupported"));
        update_session_modes_row_from_local (self);

        if (self->ext_versions_dialog)
            exm_versions_dialog_finish (self->ext_versions_dialog, FALSE);

        g_clear_error (&error);
        g_free (next);
        self->version_state = EXM_INSTALL_BUTTON_STATE_UNSUPPORTED;
        return;
    }

    const gchar *shell_version;
    g_object_get (self->versions_provider, "shell-version", &shell_version, NULL);

    for (guint i = 0; i < g_list_model_get_n_items (list); i++)
    {
        ExmVersionResult *item = EXM_VERSION_RESULT (g_list_model_get_object (list, i));

        gboolean is_compatible = exm_version_result_supports_shell_version (item, shell_version);

        if (self->ext_versions_dialog)
        {
            gint              v       = 0;
            gchar            *vn      = NULL;
            gchar            *created = NULL;
            ExmShellVersions *sv      = NULL;

            g_object_get (item,
                          "version",      &v,
                          "version_name", &vn,
                          "created",      &created,
                          "shell_versions", &sv,
                          NULL);

            exm_versions_dialog_add_release (self->ext_versions_dialog,
                                             v, vn, created, sv);

            g_clear_pointer (&sv, exm_shell_versions_unref);
            g_free (vn);
            g_free (created);
        }

        if (is_compatible)
        {
            if (compatible_version)
                g_object_unref (compatible_version);
            compatible_version = item;
        }
        else
        {
            g_object_unref (item);
        }
    }

    if (compatible_version)
    {
        gint version;
        gchar *version_name = NULL;
        gchar **session_modes;

        g_object_get (compatible_version,
                      "version", &version,
                      "version_name", &version_name,
                      "session_modes", &session_modes,
                      NULL);

        install_state = is_installed ? EXM_INSTALL_BUTTON_STATE_INSTALLED
                                     : EXM_INSTALL_BUTTON_STATE_DEFAULT;

        g_object_set (self->ext_install, "state", install_state, NULL);
        if (!version_name)
            version_name = g_strdup_printf ("%d", version);
        gtk_label_set_label (self->version_label, version_name);
        update_session_modes_row (self, session_modes);

        if (self->ext_versions_dialog)
            exm_versions_dialog_set_compatible_release (self->ext_versions_dialog, version);

        /* Save next URL so the "Load More" button can fetch additional pages */
        g_free (self->versions_next_url);
        self->versions_next_url = next;
        next = NULL;

        self->version_state = EXM_INSTALL_BUTTON_STATE_DEFAULT;
        g_free (version_name);
        g_object_unref (compatible_version);
    }
    else if (next != NULL)
    {
        exm_versions_provider_query_next_async (self->versions_provider,
                                                next,
                                                self->resolver_cancel,
                                                on_version_loaded,
                                                self);
        g_free (next);
        g_object_unref (list);
        return;
    }
    else
    {
        install_state = is_installed ? EXM_INSTALL_BUTTON_STATE_INSTALLED
                                     : EXM_INSTALL_BUTTON_STATE_UNSUPPORTED;

        g_object_set (self->ext_install, "state", install_state, NULL);
        gtk_label_set_label (self->version_label, _("Unsupported"));

        if (!update_session_modes_row_from_local (self))
        {
            guint n_items = g_list_model_get_n_items (list);

            if (n_items > 0)
            {
                ExmVersionResult *latest = EXM_VERSION_RESULT (g_list_model_get_object (list, n_items - 1));
                gchar **session_modes = NULL;

                g_object_get (latest, "session_modes", &session_modes, NULL);
                update_session_modes_row (self, session_modes);

                g_strfreev (session_modes);
                g_object_unref (latest);
            }
        }

        self->version_state = EXM_INSTALL_BUTTON_STATE_UNSUPPORTED;
        g_clear_pointer (&self->versions_next_url, g_free);
    }

    if (self->ext_versions_dialog)
        exm_versions_dialog_finish (self->ext_versions_dialog, self->versions_next_url != NULL);

    g_object_unref (list);
    g_free (next);
}


static void
populate_local_only (ExmDetailView *self)
{
    ExmExtension *ext = exm_manager_get_by_uuid (self->manager, self->uuid);

    gtk_image_set_from_icon_name (self->ext_icon, "puzzle-piece-symbolic");

    if (ext)
    {
        gchar *version_name = NULL, *version = NULL;
        g_object_get (ext,
                      "version-name", &version_name,
                      "version",      &version,
                      NULL);

        const gchar *v = (version_name && version_name[0]) ? version_name
                       : (version      && version[0])      ? version
                       : _("Unknown");
        gtk_label_set_label (self->version_label, v);

        update_session_modes_row_from_local (self);

        g_free (version_name);
        g_free (version);
    }

    gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (self->version_row), FALSE);
    gtk_widget_set_visible (GTK_WIDGET (self->version_row_go_next), FALSE);
    gtk_widget_set_visible (GTK_WIDGET (self->downloads_row), FALSE);
    gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot_container), FALSE);
    gtk_widget_set_visible (GTK_WIDGET (self->link_extensions), FALSE);
    gtk_widget_set_visible (GTK_WIDGET (self->comments_section), FALSE);

    {
        gchar **donation_urls = NULL;
        exm_unified_data_get_donation_urls (self->data, &donation_urls);
        update_donation_rows (self, donation_urls);
        g_strfreev (donation_urls);
    }

    gboolean has_donations = gtk_widget_get_visible (GTK_WIDGET (self->link_donation)) ||
                             gtk_widget_get_visible (GTK_WIDGET (self->links_donations));
    gboolean has_link = gtk_widget_get_visible (GTK_WIDGET (self->link_homepage)) || has_donations;
    gtk_widget_set_visible (GTK_WIDGET (self->links_group), has_link);

    update_tools_stack (self);
    exm_detail_view_update (self);
}

static void
on_data_loaded (GObject      *source,
                GAsyncResult *result,
                gpointer      user_data)
{
    ExmSearchResult *data;
    GError *error = NULL;
    ExmDetailView *self;

    data = exm_data_provider_get_finish (EXM_DATA_PROVIDER (source), result, &error);
    self = EXM_DETAIL_VIEW (user_data);

    if (error)
    {
        if (!exm_manager_is_installed_uuid (self->manager, self->uuid))
        {
            if (error->domain == g_quark_try_string ("request-error-quark") && error->code == 404)
            {
                gtk_stack_set_visible_child_name (self->stack, "page_empty");
            }
            else
            {
                adw_status_page_set_description (self->error_status, error->message);
                gtk_stack_set_visible_child_name (self->stack, "page_error");
            }
        }
        else
        {
            populate_local_only (self);
        }

        g_clear_error (&error);

        return;
    }

    if (EXM_IS_SEARCH_RESULT (data))
    {
        guint id;
        gchar *uuid, *name, *screenshot_uri, *icon_uri, *link;
        gchar **donation_urls;
        g_object_get (data,
                      "id", &id,
                      "uuid", &uuid,
                      "name", &name,
                      "screenshot", &screenshot_uri,
                      "icon", &icon_uri,
                      "donation_urls", &donation_urls,
                      "link", &link,
                      NULL);

        exm_unified_data_set_web_data (self->data, data);

        adw_navigation_page_set_title (ADW_NAVIGATION_PAGE (self), name);

        gtk_image_set_from_icon_name (self->ext_icon, "puzzle-piece-symbolic");
        gtk_widget_set_tooltip_text (GTK_WIDGET (self->ext_icon),
                                     // Translators: '%s' = extension name
                                     g_strdup_printf (_("%s Icon"), name));

        if (self->resolver_cancel)
        {
            g_cancellable_cancel (self->resolver_cancel);
            g_clear_object (&self->resolver_cancel);
        }

        if (icon_uri != NULL)
        {
            self->resolver_cancel = g_cancellable_new ();

            queue_resolve_image (self, icon_uri, self->resolver_cancel, TRUE);
        }

        if (screenshot_uri != NULL)
        {
            self->resolver_cancel = g_cancellable_new ();

            exm_screenshot_set_paintable (self->ext_screenshot, NULL);
            exm_screenshot_reset (self->ext_screenshot);

            gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot_container), TRUE);
            gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot_popout_button), FALSE);

            queue_resolve_image (self, screenshot_uri, self->resolver_cancel, FALSE);

            gtk_widget_set_tooltip_text (GTK_WIDGET (self->ext_screenshot),
                                         // Translators: '%s' = extension name
                                         g_strdup_printf (_("%s Screenshot"), name));
        }
        else
        {
            gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot_container), FALSE);
        }

        exm_versions_provider_query_async (self->versions_provider, uuid, self->resolver_cancel, on_version_loaded, self);

        update_donation_rows (self, donation_urls);

        self->uri_extensions = g_uri_resolve_relative ("https://extensions.gnome.org/",
                                                       link,
                                                       G_URI_FLAGS_NONE,
                                                       NULL);
        adw_action_row_set_subtitle (self->link_extensions, self->uri_extensions);

        g_clear_object (&self->ext_versions_dialog);
        g_clear_pointer (&self->versions_next_url, g_free);
        self->ext_versions_dialog = g_object_ref_sink (exm_versions_dialog_new ());

        {
            gchar *current_shell = NULL;
            g_object_get (self->versions_provider, "shell-version", &current_shell, NULL);
            exm_versions_dialog_set_current_version (self->ext_versions_dialog, current_shell);
            g_free (current_shell);
        }

        {
            ExmExtension *local_ext = exm_manager_get_by_uuid (self->manager, self->uuid);
            if (local_ext)
            {
                gchar *installed_ver = NULL;
                g_object_get (local_ext, "version", &installed_ver, NULL);
                if (installed_ver && installed_ver[0])
                    exm_versions_dialog_set_installed_version (self->ext_versions_dialog,
                                                               atoi (installed_ver));
                g_free (installed_ver);
            }
        }

        g_signal_connect_swapped (self->ext_versions_dialog, "load-more",
                                   G_CALLBACK (on_load_more_versions), self);

        self->id = id;

        if (self->comments_signal_id > 0)
            g_signal_handler_disconnect (self->show_more_btn, self->comments_signal_id);

        self->comments_signal_id = g_signal_connect (self->show_more_btn,
                                                     "clicked",
                                                     G_CALLBACK (show_more_comments),
                                                     self);

        queue_resolve_comments (self, id, self->resolver_cancel);

        update_tools_stack (self);

        // Reset focus and scroll position
        gtk_widget_grab_focus (GTK_WIDGET (self->ext_icon));
        gtk_adjustment_set_value (gtk_scrolled_window_get_vadjustment (self->scroll_area), 0);

        gtk_stack_set_visible_child_name (self->stack, "page_detail");

        return;
    }
}

void
exm_detail_view_load_for_uuid (ExmDetailView *self,
                               gchar         *uuid)
{
    ExmUnifiedData *data;
    ExmExtension   *local_info;

    if (g_strcmp0 (self->uuid, uuid) == 0 &&
        self->data != NULL &&
        exm_unified_data_get_web_data (self->data) != NULL)
    {
        g_free (uuid);
        return;
    }

    g_free (self->uuid);
    self->uuid = uuid;
    self->version_state = EXM_INSTALL_BUTTON_STATE_LOADING;

    gtk_widget_set_visible (GTK_WIDGET (self->update_icon), FALSE);
    gtk_widget_set_visible (GTK_WIDGET (self->out_of_date_icon), FALSE);
    adw_banner_set_revealed (self->error_banner, FALSE);
    gtk_label_set_label (self->version_label, _("Loading"));
    update_session_modes_row (self, NULL);

    data = exm_unified_data_new ();

    local_info = exm_manager_get_by_uuid (self->manager, uuid);
    if (local_info != NULL)
        exm_unified_data_set_local_data (data, local_info);

    g_set_object (&self->data, data);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_DATA]);
    g_object_unref (data);

    if (local_info != NULL)
    {
        gtk_stack_set_visible_child_name (self->stack, "page_detail");
        update_tools_stack (self);
    }
    else
    {
        gtk_stack_set_visible_child_name (self->stack, "page_spinner");
    }

    exm_detail_view_update (self);

    if (self->resolver_cancel)
    {
        g_cancellable_cancel (self->resolver_cancel);
        g_clear_object (&self->resolver_cancel);
    }

    gtk_image_set_from_icon_name (self->ext_icon, "puzzle-piece-symbolic");
    exm_screenshot_set_paintable (self->ext_screenshot, NULL);
    exm_screenshot_reset (self->ext_screenshot);
    gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot_popout_button), FALSE);
    delete_comment_tiles (self);
    gtk_stack_set_visible_child_name (self->comment_stack, "page_spinner");
    adw_action_row_set_subtitle (self->link_extensions, "");

    gtk_widget_set_visible (GTK_WIDGET (self->downloads_row), TRUE);
    gtk_widget_set_visible (GTK_WIDGET (self->comments_section), TRUE);
    gtk_widget_set_visible (GTK_WIDGET (self->links_group), TRUE);
    gtk_widget_set_visible (GTK_WIDGET (self->link_extensions), TRUE);
    gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (self->version_row), TRUE);
    gtk_widget_set_visible (GTK_WIDGET (self->version_row_go_next), TRUE);

    exm_data_provider_get_async (self->provider, uuid, NULL, on_data_loaded, self);
}

void
exm_detail_view_update (ExmDetailView *self)
{
    if (!self->uuid)
        return;

    if (exm_manager_is_installed_uuid (self->manager, self->uuid))
        g_object_set (self->ext_install, "state", EXM_INSTALL_BUTTON_STATE_INSTALLED, NULL);
    else
        g_object_set (self->ext_install, "state", self->version_state, NULL);

    update_tools_stack (self);
}

static void
show_versions (GtkWidget  *widget,
               const char *action_name G_GNUC_UNUSED,
               GVariant   *parameter G_GNUC_UNUSED)
{
    ExmDetailView *self;
    GtkWidget *toplevel;

    self = EXM_DETAIL_VIEW (widget);
    toplevel = GTK_WIDGET (gtk_widget_get_root (GTK_WIDGET (self)));

    adw_dialog_present (ADW_DIALOG (g_object_ref (self->ext_versions_dialog)), toplevel);
}

static void
show_error (GtkWidget  *widget,
            const char *action_name G_GNUC_UNUSED,
            GVariant   *parameter G_GNUC_UNUSED)
{
    ExmDetailView *self;
    GtkWidget *toplevel;
    ExmExtensionErrorDialog *dialog;
    const char *name = NULL;
    char *homepage = NULL;

    self = EXM_DETAIL_VIEW (widget);

    if (!self->error_text)
        return;

    toplevel = GTK_WIDGET (gtk_widget_get_root (GTK_WIDGET (self)));

    if (self->data)
    {
        name = exm_unified_data_get_name (self->data);
        exm_unified_data_get_homepage (self->data, &homepage);
    }

    dialog = exm_extension_error_dialog_new (name, self->error_text, homepage);
    adw_dialog_present (ADW_DIALOG (dialog), toplevel);

    g_free (homepage);
}

static void
show_files_or_copy_path (ExmDetailView *self,
           const char    *action_name G_GNUC_UNUSED,
           GVariant      *param G_GNUC_UNUSED)
{
    ExmExtension *extension;
    gboolean is_user;
    g_autofree gchar *path = NULL;

    extension = exm_manager_get_by_uuid (self->manager, self->uuid);

    g_object_get (extension,
                  "is_user", &is_user,
                  "path", &path,
                  NULL);

    if (is_user) 
    {
        GtkWidget *toplevel;
        g_autoptr (GFile) file = NULL;
        g_autoptr (GtkFileLauncher) launcher = NULL;

        toplevel = GTK_WIDGET (gtk_widget_get_root (GTK_WIDGET (self)));

        file = g_file_new_for_path (path);

        launcher = gtk_file_launcher_new (file);

        gtk_file_launcher_launch (launcher, GTK_WINDOW (toplevel), NULL, NULL, NULL);
    }
    else
    {
        GdkDisplay *display;
        GdkClipboard *clipboard;

        display = gdk_display_get_default ();
        clipboard = gdk_display_get_clipboard (display);

        gdk_clipboard_set_text (clipboard, path);

        adw_toast_overlay_add_toast (self->toast_overlay, adw_toast_new (_("Copied")));
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
    {
        uri = gtk_uri_launcher_new (self->uri_extensions);
    }
    else if (strcmp (action_name, "detail.open-homepage") == 0)
    {
        char *homepage = NULL;
        exm_unified_data_get_homepage (self->data, &homepage);
        uri = gtk_uri_launcher_new (homepage);
        g_free (homepage);
    }
    else if (strcmp (action_name, "detail.open-donations") == 0)
    {
        guint val;
        g_variant_get (param, "i", &val);
        uri = gtk_uri_launcher_new (self->uri_donations[val]);
    }
    else if (strcmp (action_name, "detail.open-donation") == 0)
    {
        uri = gtk_uri_launcher_new (self->uri_donations[0]);
    }
    else
    {
        g_critical ("open_link() invalid action: %s", action_name);
    }

    gtk_uri_launcher_launch (uri, GTK_WINDOW (toplevel), NULL, NULL, NULL);
}

static void
page_open_prefs (GSimpleAction *action G_GNUC_UNUSED,
                 GVariant      *param G_GNUC_UNUSED,
                 ExmDetailView *self)
{
    if (!self->uuid || !self->manager)
        return;

    ExmExtension *extension = exm_manager_get_by_uuid (self->manager, self->uuid);
    if (extension)
        exm_manager_open_prefs (self->manager, extension);
}

static void
page_remove (GSimpleAction *action G_GNUC_UNUSED,
             GVariant      *param G_GNUC_UNUSED,
             ExmDetailView *self)
{
    if (!self->uuid || !self->manager)
        return;

    ExmExtension *extension = exm_manager_get_by_uuid (self->manager, self->uuid);
    if (extension)
        exm_manager_remove_extension (self->manager, extension);
}

static gboolean
on_toggle_changed (GtkSwitch     *toggle,
                   gboolean       state,
                   ExmDetailView *self)
{
    ExmExtension *extension;

    if (!self->uuid || !self->manager)
        return FALSE;

    extension = exm_manager_get_by_uuid (self->manager, self->uuid);
    if (!extension)
        return FALSE;

    return exm_extension_apply_toggle (self->manager, extension, toggle, state);
}

static void
update_tools_stack (ExmDetailView *self)
{
    gboolean is_installed;

    if (!self->uuid || !self->manager)
        return;

    is_installed = exm_manager_is_installed_uuid (self->manager, self->uuid);

    gtk_stack_set_visible_child_name (self->tools_stack,
                                      is_installed ? "installed-tools-page"
                                                   : "uninstalled-tools-page");

    if (is_installed)
    {
        ExmExtension *extension;

        extension = exm_manager_get_by_uuid (self->manager, self->uuid);

        if (extension)
        {
            gboolean has_prefs, is_user, has_update;
            ExmExtensionState state;
            gchar *path = NULL;
            char *error_msg = NULL;
            g_object_get (extension,
                          "has-prefs",  &has_prefs,
                          "is-user",    &is_user,
                          "state",      &state,
                          "has-update", &has_update,
                          "path",       &path,
                          "error",      &error_msg,
                          NULL);

            gtk_widget_set_visible (GTK_WIDGET (self->prefs_btn), has_prefs);

            gtk_widget_set_visible (GTK_WIDGET (self->remove_btn), is_user);

            gtk_widget_set_visible (GTK_WIDGET (self->update_icon), has_update);

            gtk_widget_set_visible (GTK_WIDGET (self->out_of_date_icon),
                                    state == EXM_EXTENSION_STATE_OUT_OF_DATE);

            adw_action_row_set_subtitle(self->location_row, path);
            gtk_widget_set_visible (GTK_WIDGET (self->location_row), TRUE);
            gtk_widget_set_visible (GTK_WIDGET (self->location_row_folder_icon), is_user);
            gtk_widget_set_visible (GTK_WIDGET (self->location_row_copy_icon), !is_user);

            {
                gboolean has_error = (error_msg != NULL) && (error_msg[0] != '\0');

                g_clear_pointer (&self->error_text, g_free);
                self->error_text = has_error ? g_strdup (error_msg) : NULL;
                adw_banner_set_revealed (self->error_banner, has_error);
            }
            g_free (path);
            g_free (error_msg);

            exm_extension_bind_toggle (extension, self->ext_toggle);
        }
    }
    else
    {
        gtk_widget_set_visible (GTK_WIDGET (self->update_icon), FALSE);
        gtk_widget_set_visible (GTK_WIDGET (self->out_of_date_icon), FALSE);
        adw_banner_set_revealed (self->error_banner, FALSE);

        gtk_widget_set_visible (GTK_WIDGET (self->location_row), FALSE);
    }
}

static void
on_bind_manager (ExmDetailView *self)
{
    GListModel *ext_model;
    const gchar *shell_version;

    g_object_get (self->manager,
                  "extensions", &ext_model,
                  "shell-version", &shell_version,
                  NULL);

    g_object_set (self->versions_provider, "shell-version", shell_version, NULL);

    if (self->install_signal_id > 0)
        g_signal_handler_disconnect (self->ext_install, self->install_signal_id);

    self->install_signal_id = g_signal_connect (self->manager,
                                                "install-status",
                                                G_CALLBACK (on_install_status),
                                                self);

    g_signal_connect_object (ext_model,
                             "items-changed",
                             G_CALLBACK (exm_detail_view_update),
                             self,
                             G_CONNECT_SWAPPED);

    g_object_unref (ext_model);
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

    gtk_revealer_set_reveal_child (self->title_revealer,
                                   gtk_adjustment_get_value (adj) > 0);
}

static void
exm_detail_view_class_init (ExmDetailViewClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose      = exm_detail_view_dispose;
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

    properties [PROP_SCREENSHOT]
        = g_param_spec_object ("screenshot",
                               "Screenshot",
                               "Screenshot",
                               GDK_TYPE_PAINTABLE,
                               G_PARAM_READWRITE);

    properties [PROP_DATA]
        = g_param_spec_object ("data",
                               "Data",
                               "Unified extension data",
                               EXM_TYPE_UNIFIED_DATA,
                               G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-detail-view.ui", RESOURCE_PATH));

    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, toast_overlay);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, header_bar);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, title_revealer);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, stack);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, error_label);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, error_status);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_title);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_author);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_description);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, tools_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_install);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, remove_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, prefs_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_toggle);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_screenshot);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_screenshot_container);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_screenshot_popout_button);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, downloads_row);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, version_row);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, version_row_go_next);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, update_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, out_of_date_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, version_label);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, session_modes_row);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, session_modes_label);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, location_row);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, location_row_folder_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, location_row_copy_icon);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, error_banner);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, links_group);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, comments_section);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, link_homepage);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, link_donation);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, links_donations);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, link_extensions);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, scroll_area);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, comment_box);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, comment_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, show_more_btn);

    gtk_widget_class_bind_template_callback (widget_class, breakpoint_apply_cb);
    gtk_widget_class_bind_template_callback (widget_class, breakpoint_unapply_cb);
    gtk_widget_class_bind_template_callback (widget_class, install_remote);
    gtk_widget_class_bind_template_callback (widget_class, on_toggle_changed);
    gtk_widget_class_bind_template_callback (widget_class, format_downloads);
    gtk_widget_class_bind_template_callback (widget_class, has_homepage);
    gtk_widget_class_bind_template_callback (widget_class, format_author_link);
    gtk_widget_class_bind_template_callback (widget_class, on_author_link_activated);

    gtk_widget_class_install_action (widget_class, "detail.show-versions", NULL, show_versions);
    gtk_widget_class_install_action (widget_class, "detail.show-error", NULL, show_error);
    gtk_widget_class_install_action (widget_class, "detail.show-files-or-copy-path", NULL, (GtkWidgetActionActivateFunc) show_files_or_copy_path);
    gtk_widget_class_install_action (widget_class, "detail.open-extensions", NULL, (GtkWidgetActionActivateFunc) open_link);
    gtk_widget_class_install_action (widget_class, "detail.open-homepage", NULL, (GtkWidgetActionActivateFunc) open_link);
    gtk_widget_class_install_action (widget_class, "detail.open-donations", "i", (GtkWidgetActionActivateFunc) open_link);
    gtk_widget_class_install_action (widget_class, "detail.open-donation", NULL, (GtkWidgetActionActivateFunc) open_link);
}

static void
exm_detail_view_init (ExmDetailView *self)
{
    GtkAdjustment *adj;

    GSimpleAction *open_prefs_action;
    GSimpleAction *remove_action;

    g_type_ensure (EXM_TYPE_INSTALL_BUTTON);
    g_type_ensure (EXM_TYPE_SCREENSHOT);

    gtk_widget_init_template (GTK_WIDGET (self));

    self->provider = exm_data_provider_new ();
    self->resolver = exm_image_resolver_new ();
    self->versions_provider = exm_versions_provider_new ();
    self->comment_provider = exm_comment_provider_new ();

    adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (self->scroll_area));

    g_signal_connect_swapped (adj, "value-changed", G_CALLBACK (update_headerbar_cb), self);

    update_headerbar_cb (self);

    self->action_group = g_simple_action_group_new ();

    open_prefs_action = g_simple_action_new ("open-prefs", NULL);
    g_signal_connect (open_prefs_action, "activate", G_CALLBACK (page_open_prefs), self);

    remove_action = g_simple_action_new ("remove", NULL);
    g_signal_connect (remove_action, "activate", G_CALLBACK (page_remove), self);

    g_action_map_add_action (G_ACTION_MAP (self->action_group), G_ACTION (open_prefs_action));
    g_action_map_add_action (G_ACTION_MAP (self->action_group), G_ACTION (remove_action));

    gtk_widget_insert_action_group (GTK_WIDGET (self), "page", G_ACTION_GROUP (self->action_group));
}
