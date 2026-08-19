/*
 * exm-author-page.c
 *
 * Copyright 2022-2026 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#include "exm-author-page.h"

#include "exm-config.h"
#include "exm-search-row.h"
#include "local/exm-manager.h"
#include "web/exm-image-resolver.h"
#include "web/exm-profile-provider.h"
#include "web/exm-search-provider.h"
#include "web/model/exm-search-result.h"
#include "web/model/exm-user-profile.h"

#include <glib/gi18n.h>

struct _ExmAuthorPage
{
    AdwNavigationPage parent_instance;

    ExmManager *manager;

    ExmSearchProvider  *search_provider;
    ExmProfileProvider *profile_provider;
    ExmImageResolver   *image_resolver;

    GCancellable *search_cancellable;
    GCancellable *avatar_cancellable;

    gchar *username;
    guint creator_id;

    GListStore *extensions_model;

    // Template widgets
    AdwBreakpoint     *breakpoint;
    AdwAvatar         *avatar;
    GtkStack          *stack;
    GtkListBox        *extensions_list;
    GtkLabel          *error_label;
    GtkRevealer       *title_revealer;
    GtkLabel          *author_name;
    GtkScrolledWindow *scroll_area;
};

G_DEFINE_FINAL_TYPE (ExmAuthorPage, exm_author_page, ADW_TYPE_NAVIGATION_PAGE)

enum {
    PROP_0,
    PROP_MANAGER,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmAuthorPage *
exm_author_page_new (void)
{
    return g_object_new (EXM_TYPE_AUTHOR_PAGE, NULL);
}

static void
exm_author_page_dispose (GObject *object)
{
    ExmAuthorPage *self = EXM_AUTHOR_PAGE (object);

    if (self->search_cancellable)
    {
        g_cancellable_cancel (self->search_cancellable);
        g_clear_object (&self->search_cancellable);
    }

    if (self->avatar_cancellable)
    {
        g_cancellable_cancel (self->avatar_cancellable);
        g_clear_object (&self->avatar_cancellable);
    }

    g_clear_pointer (&self->username, g_free);
    g_clear_object (&self->extensions_model);
    g_clear_object (&self->search_provider);
    g_clear_object (&self->profile_provider);
    g_clear_object (&self->image_resolver);

    G_OBJECT_CLASS (exm_author_page_parent_class)->dispose (object);
}

static void
exm_author_page_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
    ExmAuthorPage *self = EXM_AUTHOR_PAGE (object);

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
exm_author_page_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
    ExmAuthorPage *self = EXM_AUTHOR_PAGE (object);

    switch (prop_id)
    {
    case PROP_MANAGER:
        self->manager = g_value_get_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
breakpoint_apply_cb (ExmAuthorPage *self)
{
    gtk_widget_remove_css_class (GTK_WIDGET (self->author_name), "title-1");
    gtk_widget_add_css_class (GTK_WIDGET (self->author_name), "title-2");
}

static void
breakpoint_unapply_cb (ExmAuthorPage *self)
{
    gtk_widget_remove_css_class (GTK_WIDGET (self->author_name), "title-2");
    gtk_widget_add_css_class (GTK_WIDGET (self->author_name), "title-1");
}

static void
update_headerbar_cb (ExmAuthorPage *self)
{
    GtkAdjustment *adj;

    adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (self->scroll_area));

    gtk_revealer_set_reveal_child (self->title_revealer,
                                   gtk_adjustment_get_value (adj) > 0);
}

static GtkWidget *
search_widget_factory (ExmSearchResult *result,
                       ExmAuthorPage   *self)
{
    ExmSearchRow *row;
    GValue value = G_VALUE_INIT;

    row = exm_search_row_new (self->manager, result);

    g_value_init (&value, G_TYPE_BOOLEAN);
    g_value_set_boolean (&value, TRUE);
    adw_breakpoint_add_setter (self->breakpoint, G_OBJECT (row), "compact", &value);

    return GTK_WIDGET (row);
}

static void
on_extensions_loaded (GObject       *source,
                      GAsyncResult  *res,
                      ExmAuthorPage *self)
{
    GError *error = NULL;
    GListModel *results;
    guint n_items;

    results = exm_search_provider_query_finish (EXM_SEARCH_PROVIDER (source), res, NULL, &error);

    if (error)
    {
        gtk_label_set_text (self->error_label, error->message);
        gtk_stack_set_visible_child_name (self->stack, "page_error");

        g_clear_error (&error);
        g_object_unref (self);
        return;
    }

    g_list_store_remove_all (self->extensions_model);

    n_items = results ? g_list_model_get_n_items (results) : 0;

    for (guint i = 0; i < n_items; i++)
    {
        ExmSearchResult *result = EXM_SEARCH_RESULT (g_list_model_get_item (results, i));
        gchar *username = NULL;

        g_object_get (result, "creator-username", &username, NULL);

        if (g_strcmp0 (username, self->username) == 0)
            g_list_store_append (self->extensions_model, result);

        g_free (username);
        g_object_unref (result);
    }

    g_clear_object (&results);

    gtk_stack_set_visible_child_name (self->stack,
        g_list_model_get_n_items (G_LIST_MODEL (self->extensions_model)) > 0
            ? "page_list" : "page_empty");

    g_object_unref (self);
}

static void
on_avatar_resolved (GObject       *source,
                    GAsyncResult  *res,
                    ExmAuthorPage *self)
{
    GError *error = NULL;
    GdkPaintable *paintable = exm_image_resolver_resolve_finish (EXM_IMAGE_RESOLVER (source),
                                                                  res, &error);

    if (error)
    {
        g_clear_error (&error);
        g_object_unref (self);
        return;
    }

    adw_avatar_set_custom_image (self->avatar, paintable);
    g_object_unref (paintable);

    g_object_unref (self);
}

static void
on_profile_loaded (GObject       *source,
                   GAsyncResult  *res,
                   ExmAuthorPage *self)
{
    GError *error = NULL;
    ExmUserProfile *profile;
    gchar *avatar_url = NULL;

    profile = exm_profile_provider_get_finish (EXM_PROFILE_PROVIDER (source), res, &error);

    if (error)
    {
        g_clear_error (&error);
        g_object_unref (self);
        return;
    }

    g_object_get (profile, "avatar", &avatar_url, NULL);
    g_object_unref (profile);

    if (avatar_url != NULL)
        exm_image_resolver_resolve_async (self->image_resolver, avatar_url,
                                          self->avatar_cancellable,
                                          (GAsyncReadyCallback) on_avatar_resolved,
                                          g_object_ref (self));

    g_free (avatar_url);
    g_object_unref (self);
}

void
exm_author_page_load (ExmAuthorPage *self,
                      guint          creator_id,
                      const gchar   *username,
                      const gchar   *display_name)
{
    g_return_if_fail (username != NULL);

    self->creator_id = creator_id;

    g_free (self->username);
    self->username = g_strdup (username);

    adw_avatar_set_text (self->avatar, display_name);
    adw_avatar_set_custom_image (self->avatar, NULL);

    gtk_widget_grab_focus (GTK_WIDGET (self->avatar));

    g_list_store_remove_all (self->extensions_model);
    gtk_stack_set_visible_child_name (self->stack, "page_spinner");

    if (self->search_cancellable)
    {
        g_cancellable_cancel (self->search_cancellable);
        g_clear_object (&self->search_cancellable);
    }
    self->search_cancellable = g_cancellable_new ();

    if (self->avatar_cancellable)
    {
        g_cancellable_cancel (self->avatar_cancellable);
        g_clear_object (&self->avatar_cancellable);
    }
    self->avatar_cancellable = g_cancellable_new ();

    exm_search_provider_query_by_creator_async (self->search_provider, username,
                                                self->search_cancellable,
                                                (GAsyncReadyCallback) on_extensions_loaded,
                                                g_object_ref (self));

    exm_profile_provider_get_async (self->profile_provider, creator_id,
                                    self->avatar_cancellable,
                                    (GAsyncReadyCallback) on_profile_loaded,
                                    g_object_ref (self));
}

static void
exm_author_page_class_init (ExmAuthorPageClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose      = exm_author_page_dispose;
    object_class->get_property = exm_author_page_get_property;
    object_class->set_property = exm_author_page_set_property;

    properties [PROP_MANAGER]
        = g_param_spec_object ("manager",
                               "Manager",
                               "Manager",
                               EXM_TYPE_MANAGER,
                               G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-author-page.ui", RESOURCE_PATH));

    gtk_widget_class_bind_template_child (widget_class, ExmAuthorPage, breakpoint);
    gtk_widget_class_bind_template_child (widget_class, ExmAuthorPage, avatar);
    gtk_widget_class_bind_template_child (widget_class, ExmAuthorPage, stack);
    gtk_widget_class_bind_template_child (widget_class, ExmAuthorPage, extensions_list);
    gtk_widget_class_bind_template_child (widget_class, ExmAuthorPage, error_label);
    gtk_widget_class_bind_template_child (widget_class, ExmAuthorPage, title_revealer);
    gtk_widget_class_bind_template_child (widget_class, ExmAuthorPage, author_name);
    gtk_widget_class_bind_template_child (widget_class, ExmAuthorPage, scroll_area);

    gtk_widget_class_bind_template_callback (widget_class, breakpoint_apply_cb);
    gtk_widget_class_bind_template_callback (widget_class, breakpoint_unapply_cb);
}

static void
exm_author_page_init (ExmAuthorPage *self)
{
    GtkAdjustment *adj;

    gtk_widget_init_template (GTK_WIDGET (self));

    self->search_provider = exm_search_provider_new ();
    self->profile_provider = exm_profile_provider_new ();
    self->image_resolver = exm_image_resolver_new ();

    self->extensions_model = g_list_store_new (EXM_TYPE_SEARCH_RESULT);

    gtk_list_box_bind_model (self->extensions_list, G_LIST_MODEL (self->extensions_model),
                             (GtkListBoxCreateWidgetFunc) search_widget_factory,
                             self, NULL);

    adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (self->scroll_area));

    g_signal_connect_swapped (adj, "value-changed", G_CALLBACK (update_headerbar_cb), self);

    update_headerbar_cb (self);
}
