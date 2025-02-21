/*
 * exm-browse-page.c
 *
 * Copyright 2022-2025 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#include "exm-config.h"
#include "exm-search-row.h"
#include "exm-utils.h"
#include "local/exm-manager.h"
#include "web/exm-image-resolver.h"
#include "web/exm-search-provider.h"
#include "web/model/exm-search-result.h"

#include <adwaita.h>
#include <glib/gi18n.h>

struct _ExmBrowsePage
{
    GtkWidget parent_instance;

    ExmSearchProvider *search;
    ExmImageResolver *resolver;
    ExmManager *manager;

    GtkStringList *suggestions;
    GListModel *search_results_model;

    int current_page;
    int max_pages;

    GCancellable *cancellable;

    // Template Widgets
    AdwBreakpoint       *breakpoint;
    AdwStatusPage       *status_page;
    GtkSearchEntry      *search_entry;
    GtkListBox          *search_results;
    GtkStack            *search_stack;
    GtkDropDown         *search_dropdown;
    GtkListBox          *more_results_list;
    AdwButtonRow        *more_results_btn;
    GtkLabel            *error_label;
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
    GtkWidget *child;
    ExmBrowsePage *self = (ExmBrowsePage *)object;

    child = gtk_widget_get_first_child (GTK_WIDGET (self));
    gtk_widget_unparent (child);

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
    GValue value = G_VALUE_INIT;

    row = exm_search_row_new (self->manager, result);

    g_value_init (&value, G_TYPE_BOOLEAN);
    g_value_set_boolean (&value, TRUE);
    adw_breakpoint_add_setter (self->breakpoint, G_OBJECT (row), "compact", &value);

    return GTK_WIDGET (row);
}

static void
update_load_more_btn (ExmBrowsePage *self)
{
    // Hide button if we are the last page
    gtk_widget_set_visible (GTK_WIDGET (self->more_results_list), self->current_page != self->max_pages);

    // Make it clickable
    gtk_widget_set_sensitive (GTK_WIDGET (self->more_results_btn), TRUE);
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
on_first_page_result (GObject       *source,
                      GAsyncResult  *res,
                      ExmBrowsePage *self)
{
    GError *error = NULL;
    GListModel *to_append;

    to_append = exm_search_provider_query_finish (EXM_SEARCH_PROVIDER (source), res, &self->max_pages, &error);

    if (error)
    {
        gtk_label_set_text (self->error_label, error->message);
        gtk_label_set_use_markup (self->error_label, TRUE);
        gtk_stack_set_visible_child_name (self->search_stack, "page_error");

        g_clear_error (&error);

        return;
    }

    if (G_IS_LIST_MODEL (to_append))
    {
        // Populate list model
        self->search_results_model = to_append;
    }

    // Refresh search
    refresh_search (self);

    update_load_more_btn (self);
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

    to_append = exm_search_provider_query_finish (EXM_SEARCH_PROVIDER (source), res, &self->max_pages, &error);

    if (G_IS_LIST_MODEL (to_append))
    {
        GtkWidget *child;
        GtkAdjustment *adjustment = NULL;
        double scroll_pos = 0.0;

        // Save scrolled window position
        child = gtk_widget_get_first_child (GTK_WIDGET (self->status_page));
        if (child != NULL && GTK_IS_SCROLLED_WINDOW (child))
        {
            adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (child));
            if (adjustment != NULL)
                scroll_pos = gtk_adjustment_get_value (adjustment);
        }

        n_items = g_list_model_get_n_items (G_LIST_MODEL (to_append));

        // Append to list model
        for (i = 0; i < n_items; i++) {
            GObject *item;

            item = g_list_model_get_object (to_append, i);
            g_list_store_append (G_LIST_STORE (self->search_results_model), item);

            if (i == 0)
               gtk_widget_grab_focus (gtk_widget_get_last_child (GTK_WIDGET (self->search_results)));
        }

        // Restore scrolled window position
        if (adjustment != NULL)
            gtk_adjustment_set_value (adjustment, scroll_pos);

        // Remove unnecessary model
        g_list_store_remove_all (G_LIST_STORE (to_append));
        g_clear_object (&to_append);
    }

    update_load_more_btn (self);
}

static void
on_load_more_results (AdwButtonRow  *row G_GNUC_UNUSED,
                      ExmBrowsePage *self)
{
    const char *query;
    ExmSearchSort sort;
    gtk_widget_set_sensitive (GTK_WIDGET (self->more_results_btn), FALSE);

    // If we have a current operation, cancel it
    g_cancellable_cancel (self->cancellable);
    self->cancellable = g_cancellable_new ();

    query = gtk_editable_get_text (GTK_EDITABLE (self->search_entry));
    sort = (ExmSearchSort) gtk_drop_down_get_selected (self->search_dropdown);
    exm_search_provider_query_async (self->search, query, ++self->current_page, sort, self->cancellable,
                                     (GAsyncReadyCallback) on_next_page_result,
                                     self);
}

static void
search (ExmBrowsePage *self,
        const gchar   *query,
        ExmSearchSort  sort)
{
    // Show Loading Indicator
    gtk_stack_set_visible_child_name (self->search_stack, "page_spinner");
    self->current_page = 1;

    if (self->search_results_model)
        g_clear_object (&self->search_results_model);

    // If we have a current operation, cancel it
    g_cancellable_cancel (self->cancellable);
    self->cancellable = g_cancellable_new ();

    exm_search_provider_query_async (self->search, query, 1, sort, self->cancellable,
                                     (GAsyncReadyCallback) on_first_page_result,
                                     self);
}

void
exm_browse_page_search (ExmBrowsePage *self,
                        const gchar   *query)
{
    gtk_editable_set_text (GTK_EDITABLE (self->search_entry), query);
}

void
exm_browse_page_focus_entry (ExmBrowsePage *self)
{
    gtk_widget_grab_focus (GTK_WIDGET (self->search_entry));
}

static void
on_search_changed (ExmBrowsePage *self)
{
    if (!gtk_widget_get_realized (GTK_WIDGET (self->search_entry)))
        return;

    const char *query = gtk_editable_get_text (GTK_EDITABLE (self->search_entry));
    ExmSearchSort sort = (ExmSearchSort) gtk_drop_down_get_selected (self->search_dropdown);
    search (self, query, sort);
}

static void
on_search_entry_realize (GtkSearchEntry *search_entry,
                         ExmBrowsePage  *self)
{
    const char *suggestion;
    int random_index;
    int num_suggestions;

    // Get random suggestion
    num_suggestions = g_list_model_get_n_items (G_LIST_MODEL (self->suggestions));
    random_index = g_random_int_range (0, num_suggestions);
    suggestion = gtk_string_list_get_string (self->suggestions, random_index);

    // Set placeholder value
    gtk_search_entry_set_placeholder_text (search_entry, suggestion);

    // Fire off a default search
    search (self, "", EXM_SEARCH_SORT_RELEVANCE);
}

static void
on_bind_manager (ExmBrowsePage *self)
{
    GListModel *ext_model;
    gchar *shell_version;

    g_object_get (self->manager,
                  "extensions", &ext_model,
                  NULL);

    g_signal_connect_swapped (ext_model,
                              "items-changed",
                              G_CALLBACK (refresh_search),
                              self);

    g_object_get (self->manager,
                  "shell-version",
                  &shell_version,
                  NULL);

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

    contents = exm_utils_read_resource (g_strdup_printf ("%s/suggestions.txt", RESOURCE_PATH), &length);
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

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-browse-page.ui", RESOURCE_PATH));
    gtk_widget_class_bind_template_child (widget_class, ExmBrowsePage, breakpoint);
    gtk_widget_class_bind_template_child (widget_class, ExmBrowsePage, status_page);
    gtk_widget_class_bind_template_child (widget_class, ExmBrowsePage, search_entry);
    gtk_widget_class_bind_template_child (widget_class, ExmBrowsePage, search_results);
    gtk_widget_class_bind_template_child (widget_class, ExmBrowsePage, search_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmBrowsePage, search_dropdown);
    gtk_widget_class_bind_template_child (widget_class, ExmBrowsePage, more_results_list);
    gtk_widget_class_bind_template_child (widget_class, ExmBrowsePage, more_results_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmBrowsePage, error_label);

    gtk_widget_class_bind_template_callback (widget_class, on_search_entry_realize);
    gtk_widget_class_bind_template_callback (widget_class, on_search_changed);
    gtk_widget_class_bind_template_callback (widget_class, on_load_more_results);
    gtk_widget_class_bind_template_callback (widget_class, on_bind_manager);

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

    g_object_unref (settings);

    // Rerun search when show unsupported is toggled
    g_signal_connect_swapped (self->search,
                              "notify::show-unsupported",
                              G_CALLBACK (on_search_changed),
                              self);

    load_suggestions (self);

    gtk_search_entry_set_key_capture_widget (self->search_entry, GTK_WIDGET (self));
}

