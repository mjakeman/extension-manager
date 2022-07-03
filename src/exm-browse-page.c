/* exm-browse-page.c
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

#include "exm-browse-page.h"

#include "exm-search-row.h"

#include "local/exm-manager.h"

#include "web/exm-search-provider.h"
#include "web/exm-image-resolver.h"

#include "web/model/exm-search-result.h"

#include "exm-config.h"
#include "exm-utils.h"

#include <glib/gi18n.h>

struct _ExmBrowsePage
{
    GtkWidget parent_instance;

    ExmSearchProvider *search;
    ExmImageResolver *resolver;
    ExmManager *manager;

    GtkStringList *suggestions;
    GListModel *search_results_model;
    gchar *shell_version;

    int current_page;
    int max_pages;

    // Template Widgets
    GtkSearchEntry      *search_entry;
    GtkListBox          *search_results;
    GtkStack            *search_stack;
    GtkDropDown         *search_dropdown;
    GtkButton           *more_results_btn;
};

G_DEFINE_FINAL_TYPE (ExmBrowsePage, exm_browse_page, GTK_TYPE_WIDGET)

enum {
    PROP_0,
    PROP_MANAGER,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmBrowsePage *
exm_browse_page_new (void)
{
    return g_object_new (EXM_TYPE_BROWSE_PAGE, NULL);
}

static void
exm_browse_page_finalize (GObject *object)
{
    ExmBrowsePage *self = (ExmBrowsePage *)object;

    G_OBJECT_CLASS (exm_browse_page_parent_class)->finalize (object);
}

static void
exm_browse_page_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
    ExmBrowsePage *self = EXM_BROWSE_PAGE (object);

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
exm_browse_page_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
    ExmBrowsePage *self = EXM_BROWSE_PAGE (object);

    switch (prop_id)
    {
    case PROP_MANAGER:
        self->manager = g_value_get_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static GtkWidget *
search_widget_factory (ExmSearchResult *result,
                       ExmBrowsePage   *self)
{
    ExmSearchRow *row;
    gchar *uuid;
    gboolean is_installed;
    gboolean is_supported;

    g_object_get (result, "uuid", &uuid, NULL);

    is_installed = exm_manager_is_installed_uuid (self->manager, uuid);
    is_supported = exm_search_result_supports_shell_version (result, self->shell_version);

    row = exm_search_row_new (result, is_installed, is_supported);

    return GTK_WIDGET (row);
}

static void
refresh_search (ExmBrowsePage *self)
{
    if (!self->manager)
        return;

    if (!self->search_results_model || g_list_model_get_n_items (self->search_results_model) == 0)
    {
        // No Results Found
        gtk_stack_set_visible_child_name (self->search_stack, "page_empty");
        return;
    }

    gtk_list_box_bind_model (self->search_results, self->search_results_model,
                             (GtkListBoxCreateWidgetFunc) search_widget_factory,
                             g_object_ref (self), g_object_unref);

    // Hide Loading Indicator
    gtk_stack_set_visible_child_name (self->search_stack, "page_results");
}

static void
on_next_page_result (GObject       *source,
                     GAsyncResult  *res,
                     ExmBrowsePage *self)
{
    GError *error = NULL;
    GListModel *to_append;
    int n_items;
    int i;

    to_append = exm_search_provider_query_finish (EXM_SEARCH_PROVIDER (source), res, &error);
    n_items = g_list_model_get_n_items (G_LIST_MODEL (to_append));

    for (i = 0; i < n_items; i++) {
        GObject *item;

        item = g_list_model_get_object (to_append, i);
        g_list_store_append (G_LIST_STORE (self->search_results_model), item);
    }

    g_list_store_remove_all (G_LIST_STORE (to_append));
    g_object_unref (to_append);
}

static void
on_load_more_results (GtkButton     *btn,
                      ExmBrowsePage *self)
{
    // Show Loading Indicator?

    const char *query = gtk_editable_get_text (GTK_EDITABLE (self->search_entry));
    ExmSearchSort sort = (ExmSearchSort) gtk_drop_down_get_selected (self->search_dropdown);
    exm_search_provider_query_async (self->search, query, ++self->current_page, sort, NULL,
                                     (GAsyncReadyCallback) on_next_page_result,
                                     self);
}

static void
on_search_result (GObject       *source,
                  GAsyncResult  *res,
                  ExmBrowsePage *self)
{
    GError *error = NULL;

    self->search_results_model = exm_search_provider_query_finish (EXM_SEARCH_PROVIDER (source), res, &error);

    refresh_search (self);
}

static void
search (ExmBrowsePage *self,
        const gchar   *query,
        ExmSearchSort  sort)
{
    // Show Loading Indicator
    gtk_stack_set_visible_child_name (self->search_stack, "page_spinner");
    self->current_page = 1;

    exm_search_provider_query_async (self->search, query, 1, sort, NULL,
                                     (GAsyncReadyCallback) on_search_result,
                                     self);
}

static void
on_search_changed (ExmBrowsePage  *self)
{
    const char *query = gtk_editable_get_text (GTK_EDITABLE (self->search_entry));
    ExmSearchSort sort = (ExmSearchSort) gtk_drop_down_get_selected (self->search_dropdown);
    search (self, query, sort);
}

static void
on_search_entry_realize (GtkSearchEntry *search_entry,
                         ExmBrowsePage  *self)
{
    const char *suggestion;
    char *fmt;
    int random_index;
    int num_suggestions;

    // Get random suggestion
    num_suggestions = g_list_model_get_n_items (self->suggestions);
    random_index = g_random_int_range (0, num_suggestions);
    suggestion = gtk_string_list_get_string (self->suggestions, random_index);

    // Translators:
    //  - '%s' is an extension e.g. Blur my Shell
    //  - Please use unicode quotation marks e.g. “” (not "")
    fmt = g_strdup_printf (_("e.g. “%s”"), suggestion);

    // Set placeholder value
    g_object_set (search_entry, "placeholder-text", fmt, NULL);

    // Fire off a default search
    search (self, "", EXM_SEARCH_SORT_POPULARITY);
    gtk_widget_grab_focus (GTK_WIDGET (search_entry));
}

static void
on_bind_manager (ExmBrowsePage *self)
{
    GListModel *user_ext_model;
    GListModel *system_ext_model;
    gchar *shell_version;

    g_object_get (self->manager,
                  "user-extensions", &user_ext_model,
                  "system-extensions", &system_ext_model,
                  NULL);

    g_signal_connect_swapped (user_ext_model,
                              "items-changed",
                              G_CALLBACK (refresh_search),
                              self);

    g_signal_connect_swapped (system_ext_model,
                              "items-changed",
                              G_CALLBACK (refresh_search),
                              self);

    g_object_get (self->manager,
                  "shell-version",
                  &shell_version,
                  NULL);

    self->shell_version = shell_version;
    g_object_set (self->search, "shell-version", shell_version, NULL);

    refresh_search (self);
}

static void
load_suggestions (ExmBrowsePage *self)
{
    char *contents;
    char *contents_nul_term;
    char **suggest_array;
    gsize length;

    contents = exm_utils_read_resource ("/com/mattjakeman/ExtensionManager/suggestions.txt", &length);
    self->suggestions = gtk_string_list_new (NULL);

    if (contents)
    {
        int iter;

        contents_nul_term = g_strndup (contents, length);

        // Load dynamically from resource
        suggest_array = g_strsplit_set (contents_nul_term, "\n", -1);

        for (iter = 0; suggest_array[iter] != NULL; iter++)
            gtk_string_list_append (self->suggestions, suggest_array[iter]);

        g_strfreev (suggest_array);
        g_free (contents_nul_term);
        g_free (contents);
    }
    else
    {
        // Hardcoded fallback suggestion
        gtk_string_list_append (self->suggestions, "Blur my Shell");
    }
}

static void
exm_browse_page_class_init (ExmBrowsePageClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_browse_page_finalize;
    object_class->get_property = exm_browse_page_get_property;
    object_class->set_property = exm_browse_page_set_property;

    properties [PROP_MANAGER]
        = g_param_spec_object ("manager",
                               "Manager",
                               "Manager",
                               EXM_TYPE_MANAGER,
                               G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-browse-page.ui");
    gtk_widget_class_bind_template_child (widget_class, ExmBrowsePage, search_entry);
    gtk_widget_class_bind_template_child (widget_class, ExmBrowsePage, search_results);
    gtk_widget_class_bind_template_child (widget_class, ExmBrowsePage, search_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmBrowsePage, search_dropdown);
    gtk_widget_class_bind_template_child (widget_class, ExmBrowsePage, more_results_btn);

    gtk_widget_class_bind_template_callback (widget_class, on_search_entry_realize);

    gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
exm_browse_page_init (ExmBrowsePage *self)
{
    GSettings *settings;
    gtk_widget_init_template (GTK_WIDGET (self));

    self->search = exm_search_provider_new ();

    settings = g_settings_new (APP_ID);

    g_settings_bind (settings, "show-unsupported",
                     self->search, "show-unsupported",
                     G_SETTINGS_BIND_GET);

    // Rerun search when show unsupported is toggled
    g_signal_connect_swapped (self->search,
                              "notify::show-unsupported",
                              G_CALLBACK (on_search_changed),
                              self);

    g_object_unref (settings);

    g_signal_connect_swapped (self->search_entry,
                              "search-changed",
                              G_CALLBACK (on_search_changed),
                              self);

    g_signal_connect_swapped (self->search_dropdown,
                              "notify::selected",
                              G_CALLBACK (on_search_changed),
                              self);

    g_signal_connect (self->search_entry,
                      "realize",
                      G_CALLBACK (on_search_entry_realize),
                      self);

    g_signal_connect (self->more_results_btn,
                      "clicked",
                      G_CALLBACK (on_load_more_results),
                      self);

    g_signal_connect (self,
                      "notify::manager",
                      G_CALLBACK (on_bind_manager),
                      NULL);

    load_suggestions (self);
}
