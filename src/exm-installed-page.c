/*
 * exm-installed-page.c
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

#include "exm-installed-page.h"

#include "exm-config.h"
#include "exm-enums.h"
#include "exm-extension-row.h"
#include "exm-types.h"
#include "exm-window.h"
#include "local/exm-manager.h"

#include <glib/gi18n.h>

struct _ExmInstalledPage
{
    GtkWidget parent_instance;

    ExmManager *manager;

    // Template Widgets
    GtkStack *stack;
    AdwBanner *updates_banner;
    AdwSwitchRow *global_toggle;
    GtkListBox *user_list_box;
    GtkListBox *system_list_box;
    GtkListBox *search_list_box;
    GtkFilterListModel *search_list_model;

    gboolean sort_enabled_first;
};

G_DEFINE_FINAL_TYPE (ExmInstalledPage, exm_installed_page, GTK_TYPE_WIDGET)

enum {
    PROP_0,
    PROP_MANAGER,
    PROP_SORT_ENABLED_FIRST,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
invalidate_model_bindings (ExmInstalledPage *self);

ExmInstalledPage *
exm_installed_page_new (void)
{
    return g_object_new (EXM_TYPE_INSTALLED_PAGE, NULL);
}

static void
exm_installed_page_finalize (GObject *object)
{
    GtkWidget *child;
    ExmInstalledPage *self = (ExmInstalledPage *)object;

    child = gtk_widget_get_first_child (GTK_WIDGET (self));
    gtk_widget_unparent (child);

    G_OBJECT_CLASS (exm_installed_page_parent_class)->finalize (object);
}

static void
exm_installed_page_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
    ExmInstalledPage *self = EXM_INSTALLED_PAGE (object);

    switch (prop_id)
    {
    case PROP_MANAGER:
        g_value_set_object (value, self->manager);
        break;
    case PROP_SORT_ENABLED_FIRST:
        g_value_set_boolean (value, self->sort_enabled_first);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_installed_page_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
    ExmInstalledPage *self = EXM_INSTALLED_PAGE (object);

    switch (prop_id)
    {
    case PROP_MANAGER:
        self->manager = g_value_get_object (value);
        break;
    case PROP_SORT_ENABLED_FIRST:
        self->sort_enabled_first = g_value_get_boolean (value);
        invalidate_model_bindings (self);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static GtkWidget *
widget_factory (ExmExtension     *extension,
                ExmInstalledPage *self)
{
    ExmExtensionRow *row;
    g_return_val_if_fail (EXM_IS_EXTENSION (extension), GTK_WIDGET (NULL));
    g_return_val_if_fail (EXM_IS_INSTALLED_PAGE (self), GTK_WIDGET (NULL));

    row = exm_extension_row_new (extension, self->manager);
    return GTK_WIDGET (row);
}

static int
compare_enabled (ExmExtension *this,
                 ExmExtension *other,
                 gpointer      user_data G_GNUC_UNUSED)
{
    g_return_val_if_fail (EXM_IS_EXTENSION (this), 2);
    g_return_val_if_fail (EXM_IS_EXTENSION (other), 2); // Crash

    ExmExtensionState this_state;
    ExmExtensionState other_state;

    g_object_get (this, "state", &this_state, NULL);
    g_object_get (other, "state", &other_state, NULL);

    gboolean this_enabled = (this_state == EXM_EXTENSION_STATE_ACTIVE);
    gboolean other_enabled = (other_state == EXM_EXTENSION_STATE_ACTIVE);

    if ((this_enabled && other_enabled) || (!this_enabled && !other_enabled))
        return 0;
    else if (this_enabled && !other_enabled)
        return -1;
    else if (!this_enabled && other_enabled)
        return 1;

    return 0; // Never reached
}

static void
on_search_changed (ExmWindow *window,
                   gpointer   user_data)
{
    GtkStringFilter *search_filter = (GtkStringFilter *)user_data;

    gtk_string_filter_set_search (search_filter, exm_window_get_search_query (window));
}

static void
on_visible_stack_changed (GObject    *object G_GNUC_UNUSED,
                          GParamSpec *pspec G_GNUC_UNUSED,
                          gpointer    user_data)
{
    ExmInstalledPage *self = (ExmInstalledPage *)user_data;
    ExmWindow *window;
    gboolean search_mode;

    window = EXM_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (self), EXM_TYPE_WINDOW));
    search_mode = exm_window_get_search_mode (window);

    exm_installed_page_show_page (search_mode, self);
}

void
exm_installed_page_show_page (gboolean          search_mode,
                              ExmInstalledPage *self)
{
    if (search_mode && g_list_model_get_n_items (G_LIST_MODEL (self->search_list_model)) > 0)
        gtk_stack_set_visible_child_name (self->stack , "page_results");
    else if (search_mode)
        gtk_stack_set_visible_child_name (self->stack , "page_empty");
    else
        gtk_stack_set_visible_child_name (self->stack , "page_list");
}

static void
bind_list_box (GListModel       *model,
               ExmInstalledPage *self)
{
    GtkExpression *expression;
    GtkStringSorter *alphabetical_sorter;
    GtkSortListModel *sorted_model;
    GtkStringFilter *search_filter;
    GtkBoolFilter *is_user_filter;
    GtkFilterListModel *filtered_model;
    ExmWindow *window;
    const char *query;

    g_return_if_fail (G_IS_LIST_MODEL (model));

    // Sort alphabetically
    expression = gtk_property_expression_new (EXM_TYPE_EXTENSION, NULL, "name");
    alphabetical_sorter = gtk_string_sorter_new (expression);

    if (self->sort_enabled_first)
    {
        GtkCustomSorter *enabled_sorter;
        GtkMultiSorter *multi_sorter;

        // Sort by enabled
        enabled_sorter = gtk_custom_sorter_new ((GCompareDataFunc) compare_enabled, NULL, NULL);

        multi_sorter = gtk_multi_sorter_new ();
        gtk_multi_sorter_append (multi_sorter, GTK_SORTER (enabled_sorter));
        gtk_multi_sorter_append (multi_sorter, GTK_SORTER (alphabetical_sorter));

        sorted_model = gtk_sort_list_model_new (model, GTK_SORTER (multi_sorter));
    }
    else
    {
        sorted_model = gtk_sort_list_model_new (model, GTK_SORTER (alphabetical_sorter));
    }

    search_filter = gtk_string_filter_new (expression);
    self->search_list_model = gtk_filter_list_model_new (G_LIST_MODEL (sorted_model), GTK_FILTER (search_filter));

    gtk_list_box_bind_model (self->search_list_box, G_LIST_MODEL (self->search_list_model),
                             (GtkListBoxCreateWidgetFunc) widget_factory,
                             self, NULL);

    // Filter by user/system extension
    expression = gtk_property_expression_new (EXM_TYPE_EXTENSION, NULL, "is-user");
    is_user_filter = gtk_bool_filter_new (expression);
    filtered_model = gtk_filter_list_model_new (G_LIST_MODEL (sorted_model), GTK_FILTER (is_user_filter));

    gtk_list_box_bind_model (self->user_list_box, G_LIST_MODEL (filtered_model),
                             (GtkListBoxCreateWidgetFunc) widget_factory,
                             self, NULL);

    is_user_filter = gtk_bool_filter_new (expression);
    gtk_bool_filter_set_invert (is_user_filter, TRUE);
    filtered_model = gtk_filter_list_model_new (G_LIST_MODEL (sorted_model), GTK_FILTER (is_user_filter));

    gtk_list_box_bind_model (self->system_list_box, G_LIST_MODEL (filtered_model),
                             (GtkListBoxCreateWidgetFunc) widget_factory,
                             self, NULL);

    window = EXM_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (self), EXM_TYPE_WINDOW));
    query = exm_window_get_search_query (window);

    // Refilter when sort-enabled-first changes and there is an ongoing search
    if (query)
        gtk_string_filter_set_search (search_filter, query);

    g_signal_connect (window,
                      "search-changed",
                      G_CALLBACK (on_search_changed),
                      search_filter);

    g_signal_connect (self->search_list_model,
                      "notify::n-items",
                      G_CALLBACK (on_visible_stack_changed),
                      self);
}

static guint
show_updates_banner (ExmInstalledPage *self)
{
    adw_banner_set_revealed (self->updates_banner, TRUE);

    return G_SOURCE_REMOVE;
}

static void
on_updates_available (ExmManager       *manager G_GNUC_UNUSED,
                      int               n_updates,
                      ExmInstalledPage *self)
{
    char *label;

    // Translators: '%d' = number of extensions that will be updated
    label = g_strdup_printf(ngettext("%d extension will be updated on next login",
                                     "%d extensions will be updated on next login",
                                     n_updates), n_updates);

    adw_banner_set_title (self->updates_banner, label);
    g_free (label);

    // Short delay to draw user attention
    g_timeout_add (500, G_SOURCE_FUNC (show_updates_banner), self);
}

static void
invalidate_model_bindings (ExmInstalledPage *self)
{
    GListModel *ext_model;

    if (!self->manager)
        return;

    g_object_get (self->manager,
                  "extensions", &ext_model,
                  NULL);

    if (ext_model)
        bind_list_box (ext_model, self);
}

static void
on_bind_manager (ExmInstalledPage *self)
{
    // Bind (or rebind) models
    invalidate_model_bindings (self);

    g_signal_connect (self->manager,
                      "updates-available",
                      G_CALLBACK (on_updates_available),
                      self);

    g_object_bind_property (self->manager,
                            "extensions-enabled",
                            self->global_toggle,
                            "active",
                            G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);

    // Check if updates are available
    // NOTE: We need to do this *after* connecting the signal
    // handler above, otherwise we will not be notified.
    exm_manager_check_for_updates (self->manager);
}

static void
exm_installed_page_class_init (ExmInstalledPageClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_installed_page_finalize;
    object_class->get_property = exm_installed_page_get_property;
    object_class->set_property = exm_installed_page_set_property;

    properties [PROP_MANAGER]
        = g_param_spec_object ("manager",
                               "Manager",
                               "Manager",
                               EXM_TYPE_MANAGER,
                               G_PARAM_READWRITE);

    properties [PROP_SORT_ENABLED_FIRST]
        = g_param_spec_boolean ("sort-enabled-first",
                                "Sort Enabled First",
                                "Sort Enabled First",
                                FALSE,
                                G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-installed-page.ui", RESOURCE_PATH));

    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, stack);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, updates_banner);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, global_toggle);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, user_list_box);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, system_list_box);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, search_list_box);

    gtk_widget_class_bind_template_callback (widget_class, on_bind_manager);

    gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
exm_installed_page_init (ExmInstalledPage *self)
{
    GSettings *settings;

    gtk_widget_init_template (GTK_WIDGET (self));

    settings = g_settings_new (APP_ID);

    g_settings_bind (settings, "sort-enabled-first",
                     self, "sort-enabled-first",
                     G_SETTINGS_BIND_GET);

    g_object_unref (settings);
}
