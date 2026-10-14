/*
 * exm-installed-page.c
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

#include "exm-installed-page.h"

#include "exm-config.h"
#include "exm-enums.h"
#include "exm-extension-row.h"
#include "exm-types.h"
#include "exm-window.h"
#include "local/exm-extension.h"
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
    AdwPreferencesGroup *user_prefs_group;
    AdwPreferencesGroup *system_prefs_group;
    AdwPreferencesGroup *search_prefs_group;
    GtkFilterListModel *search_list_model;
    GListModel *system_filtered_model;

    GtkStringFilter *search_filter;
    guint search_list_model_signal_id;

    gboolean sort_enabled_first;
    gboolean search_mode_enabled;
    char *search_query;
    GListModel *bound_model;
    guint signal_id;

    gboolean selection_mode;
    guint n_selected;
};

G_DEFINE_FINAL_TYPE (ExmInstalledPage, exm_installed_page, GTK_TYPE_WIDGET)

enum {
    PROP_0,
    PROP_MANAGER,
    PROP_SORT_ENABLED_FIRST,
    PROP_SEARCH_MODE_ENABLED,
    PROP_SEARCH_QUERY,
    PROP_SELECTION_MODE,
    PROP_N_SELECTED,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
invalidate_model_bindings (ExmInstalledPage *self);

static void
switch_page (ExmInstalledPage *self);

static void
propagate_selection_mode (ExmInstalledPage *self);

static void
update_system_group_visibility (ExmInstalledPage *self);

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

    g_clear_object (&self->system_filtered_model);
    g_clear_object (&self->search_filter);
    g_clear_object (&self->search_list_model);
    g_clear_pointer (&self->search_query, g_free);

    if (self->bound_model != NULL && self->signal_id > 0)
        g_signal_handler_disconnect (self->bound_model, self->signal_id);
    self->signal_id = 0;
    g_clear_object (&self->bound_model);

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
    case PROP_SELECTION_MODE:
        g_value_set_boolean (value, self->selection_mode);
        break;
    case PROP_N_SELECTED:
        g_value_set_uint (value, self->n_selected);
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
    case PROP_SEARCH_MODE_ENABLED:
        self->search_mode_enabled = g_value_get_boolean (value);
        switch_page (self);
        break;
    case PROP_SEARCH_QUERY:
        g_free (self->search_query);
        self->search_query = g_value_dup_string (value);
        break;
    case PROP_SELECTION_MODE:
        self->selection_mode = g_value_get_boolean (value);
        if (!self->selection_mode && self->n_selected != 0)
        {
            self->n_selected = 0;
            g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_SELECTED]);
        }
        propagate_selection_mode (self);
        update_system_group_visibility (self);
        g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTION_MODE]);
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
    g_object_set (row, "selection-mode", self->selection_mode, NULL);

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

    g_assert_not_reached ();
}

static void
on_search_changed (ExmInstalledPage *self,
                   GParamSpec       *pspec G_GNUC_UNUSED,
                   gpointer          user_data G_GNUC_UNUSED)
{
    if (self->search_filter == NULL)
        return;

    gtk_string_filter_set_search (self->search_filter, self->search_query);
}

static void
switch_page (ExmInstalledPage *self)
{
    if (self->search_mode_enabled
        && g_list_model_get_n_items (G_LIST_MODEL (self->search_list_model)) > 0)
        gtk_stack_set_visible_child_name (self->stack , "page_results");
    else if (self->search_mode_enabled)
        gtk_stack_set_visible_child_name (self->stack , "page_no_results");
    else if (gtk_widget_get_visible (GTK_WIDGET (self->user_prefs_group))
             || gtk_widget_get_visible (GTK_WIDGET (self->system_prefs_group)))
        gtk_stack_set_visible_child_name (self->stack, "page_list");
    else
        gtk_stack_set_visible_child_name (self->stack , "page_empty");
}

static void
on_visible_stack_changed (GObject    *object G_GNUC_UNUSED,
                          GParamSpec *pspec G_GNUC_UNUSED,
                          gpointer    user_data)
{
    ExmInstalledPage *self = EXM_INSTALLED_PAGE (user_data);

    switch_page (self);
}

static void
update_system_group_visibility (ExmInstalledPage *self)
{
    guint n_items;

    if (!self->system_filtered_model)
        return;

    n_items = g_list_model_get_n_items (self->system_filtered_model);
    gtk_widget_set_visible (GTK_WIDGET (self->system_prefs_group),
                            n_items > 0 && !self->selection_mode);
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

    g_return_if_fail (G_IS_LIST_MODEL (model));

    if (self->search_list_model != NULL && self->search_list_model_signal_id > 0)
        g_signal_handler_disconnect (self->search_list_model, self->search_list_model_signal_id);
    self->search_list_model_signal_id = 0;
    g_clear_object (&self->search_list_model);

    // Sort alphabetically
    expression = gtk_expression_ref (gtk_property_expression_new (EXM_TYPE_EXTENSION, NULL, "name"));
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
    g_set_object (&self->search_filter, search_filter);
    self->search_list_model = gtk_filter_list_model_new (g_object_ref (G_LIST_MODEL (sorted_model)), GTK_FILTER (search_filter));

    adw_preferences_group_bind_model (self->search_prefs_group, G_LIST_MODEL (self->search_list_model),
                                      (GtkListBoxCreateWidgetFunc) widget_factory,
                                      self, NULL);

    // Filter by user/system extension
    expression = gtk_expression_ref (gtk_property_expression_new (EXM_TYPE_EXTENSION, NULL, "is-user"));
    is_user_filter = gtk_bool_filter_new (expression);
    filtered_model = gtk_filter_list_model_new (g_object_ref (G_LIST_MODEL (sorted_model)), GTK_FILTER (is_user_filter));

    g_object_bind_property (filtered_model, "n-items",
                            self->user_prefs_group, "visible",
                            G_BINDING_SYNC_CREATE);

    adw_preferences_group_bind_model (self->user_prefs_group, G_LIST_MODEL (filtered_model),
                                      (GtkListBoxCreateWidgetFunc) widget_factory,
                                      self, NULL);
    g_object_unref (filtered_model);

    is_user_filter = gtk_bool_filter_new (expression);
    gtk_bool_filter_set_invert (is_user_filter, TRUE);
    filtered_model = gtk_filter_list_model_new (G_LIST_MODEL (sorted_model), GTK_FILTER (is_user_filter));

    if (self->system_filtered_model != NULL)
        g_signal_handlers_disconnect_by_func (self->system_filtered_model,
                                              update_system_group_visibility, self);

    g_set_object (&self->system_filtered_model, G_LIST_MODEL (filtered_model));
    g_signal_connect_swapped (self->system_filtered_model, "notify::n-items",
                              G_CALLBACK (update_system_group_visibility), self);
    update_system_group_visibility (self);

    adw_preferences_group_bind_model (self->system_prefs_group, G_LIST_MODEL (filtered_model),
                                      (GtkListBoxCreateWidgetFunc) widget_factory,
                                      self, NULL);
    g_object_unref (filtered_model);

    // Refilter when sort-enabled-first changes and there is an ongoing search
    if (self->search_query)
        gtk_string_filter_set_search (search_filter, self->search_query);

    self->search_list_model_signal_id = g_signal_connect (self->search_list_model,
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

static gboolean
focus_matching_extension (AdwPreferencesGroup *prefs_group,
                          ExmExtension        *extension)
{
    int index = 0;
    ExmExtensionRow *row;

    while ((row = EXM_EXTENSION_ROW (adw_preferences_group_get_row (prefs_group, index))))
    {
        ExmExtension *row_extension;

        g_object_get (row, "extension", &row_extension, NULL);

        if (is_extension_equal (extension, row_extension))
        {
            exm_search_row_focus_toggle (row);
            g_object_unref (row_extension);

            return TRUE;
        }

        g_object_unref (row_extension);
        index++;
    }

    return FALSE;
}

static void
on_extensions_changed (GListModel       *model,
                       guint             position,
                       guint             removed,
                       guint             added,
                       ExmInstalledPage *self)
{
    if (!self->sort_enabled_first || (removed > 0 && added > 0))
        return;

    if (position >= g_list_model_get_n_items (model))
        return;

    ExmExtension *extension = EXM_EXTENSION (g_list_model_get_object (model, position));
    if (extension == NULL)
        return;

    GtkRoot *toplevel = gtk_widget_get_root (GTK_WIDGET (self));
    if (toplevel == NULL)
    {
        g_object_unref (extension);
        return;
    }

    GtkWidget *focused_widget = gtk_window_get_focus (GTK_WINDOW (toplevel));
    if (focused_widget == NULL)
    {
        g_object_unref (extension);
        return;
    }

    if (g_list_store_find_with_equal_func (G_LIST_STORE (model), extension, (GEqualFunc)is_extension_equal, &position))
        g_list_model_items_changed (model, position, 1, 1);

    if (gtk_widget_has_focus (focused_widget)
        && gtk_widget_get_child_visible (GTK_WIDGET (self)))
    {
        if (g_strcmp0 (gtk_stack_get_visible_child_name (self->stack), "page_results") == 0)
        {
            focus_matching_extension (self->search_prefs_group, extension);
        }
        else
        {
            if (!focus_matching_extension (self->user_prefs_group, extension))
                focus_matching_extension (self->system_prefs_group, extension);
        }
    }

    g_object_unref (extension);
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
    {
        bind_list_box (g_object_ref (ext_model), self);

        if (self->bound_model != NULL && self->signal_id > 0)
            g_signal_handler_disconnect (self->bound_model, self->signal_id);
        g_clear_object (&self->bound_model);

        self->bound_model = ext_model;
        self->signal_id = g_signal_connect (ext_model,
                                            "items-changed",
                                            G_CALLBACK (on_extensions_changed),
                                            self);
    }
}

static void
foreach_extension_row (AdwPreferencesGroup   *group,
                       void                 (*fn)(ExmExtensionRow *, gpointer),
                       gpointer               user_data)
{
    guint i = 0;
    ExmExtensionRow *row;
    while ((row = EXM_EXTENSION_ROW (adw_preferences_group_get_row (group, i++))) != NULL)
        fn (row, user_data);
}

static void
set_row_selection_mode (ExmExtensionRow *row,
                        gpointer         user_data)
{
    gboolean mode = GPOINTER_TO_INT (user_data);
    g_object_set (row, "selection-mode", mode, NULL);
}

static void
propagate_selection_mode (ExmInstalledPage *self)
{
    gpointer mode = GINT_TO_POINTER (self->selection_mode);
    foreach_extension_row (self->user_prefs_group,   set_row_selection_mode, mode);
    foreach_extension_row (self->search_prefs_group, set_row_selection_mode, mode);
}

static guint
count_selected_rows (ExmInstalledPage *self)
{
    guint count = 0;
    guint i;
    ExmExtensionRow *row;
    AdwPreferencesGroup *groups[] = {
        self->user_prefs_group,
        self->search_prefs_group,
    };

    for (guint g = 0; g < G_N_ELEMENTS (groups); g++)
    {
        i = 0;
        while ((row = EXM_EXTENSION_ROW (adw_preferences_group_get_row (groups[g], i++))) != NULL)
            if (exm_extension_row_is_selected (row))
                count++;
    }
    return count;
}

static void
on_page_selection_changed (GSimpleAction    *action G_GNUC_UNUSED,
                           GVariant         *param G_GNUC_UNUSED,
                           ExmInstalledPage *self)
{
    guint count = count_selected_rows (self);
    if (count != self->n_selected)
    {
        self->n_selected = count;
        g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_SELECTED]);
    }
}

void
exm_installed_page_uninstall_selected (ExmInstalledPage *self)
{
    GHashTable *uuids_to_remove;
    GHashTableIter iter;
    gpointer key;
    AdwPreferencesGroup *groups[] = {
        self->user_prefs_group,
        self->search_prefs_group,
    };

    g_return_if_fail (EXM_IS_INSTALLED_PAGE (self));

    uuids_to_remove = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

    for (guint g = 0; g < G_N_ELEMENTS (groups); g++)
    {
        guint i = 0;
        ExmExtensionRow *row;
        while ((row = EXM_EXTENSION_ROW (adw_preferences_group_get_row (groups[g], i++))) != NULL)
        {
            ExmExtension *ext;
            gchar *uuid;

            if (!exm_extension_row_is_selected (row))
                continue;

            ext = NULL;
            g_object_get (row, "extension", &ext, NULL);
            if (!ext) continue;

            uuid = NULL;
            g_object_get (ext, "uuid", &uuid, NULL);
            g_object_unref (ext);

            if (uuid)
                g_hash_table_add (uuids_to_remove, uuid);
        }
    }

    g_hash_table_iter_init (&iter, uuids_to_remove);
    while (g_hash_table_iter_next (&iter, &key, NULL))
    {
        ExmExtension *ext = exm_manager_get_by_uuid (self->manager, (const char *) key);
        if (ext)
        {
            exm_manager_remove_extension (self->manager, ext);
            g_object_unref (ext);
        }
    }

    g_hash_table_destroy (uuids_to_remove);
}

static void
check_for_updates_if_supported (ExmInstalledPage *self)
{
    gboolean is_supported = FALSE;

    g_object_get (self->manager, "is-supported", &is_supported, NULL);

    if (is_supported)
        exm_manager_check_for_updates (self->manager);
}

static void
on_bind_manager (ExmInstalledPage *self)
{
    // Bind (or rebind) models
    invalidate_model_bindings (self);

    g_signal_connect_object (self->manager, "notify::extensions",
                             G_CALLBACK (invalidate_model_bindings), self,
                             G_CONNECT_SWAPPED);

    g_signal_connect (self->manager,
                      "updates-available",
                      G_CALLBACK (on_updates_available),
                      self);

    g_object_bind_property (self->manager,
                            "extensions-enabled",
                            self->global_toggle,
                            "active",
                            G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);

    switch_page (self);

    // Check if updates are available
    // NOTE: We need to do this *after* connecting the signal
    // handler above, otherwise we will not be notified.
    g_signal_connect_object (self->manager, "notify::is-supported",
                             G_CALLBACK (check_for_updates_if_supported), self,
                             G_CONNECT_SWAPPED);

    check_for_updates_if_supported (self);
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

    properties [PROP_SEARCH_MODE_ENABLED]
        = g_param_spec_boolean ("search-mode-enabled",
                                "Search Mode Enabled",
                                "Search Mode Enabled",
                                FALSE,
                                G_PARAM_READWRITE);

    properties [PROP_SEARCH_QUERY] =
        g_param_spec_string ("search-query",
                             "Search Query",
                             "Search Query",
                             NULL,
                             G_PARAM_READWRITE);

    properties [PROP_SELECTION_MODE]
        = g_param_spec_boolean ("selection-mode",
                                "Selection Mode",
                                "Whether bulk-selection mode is active",
                                FALSE,
                                G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

    properties [PROP_N_SELECTED]
        = g_param_spec_uint ("n-selected",
                             "N Selected",
                             "Number of extensions currently selected",
                             0, G_MAXUINT, 0,
                             G_PARAM_READABLE|G_PARAM_EXPLICIT_NOTIFY);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-installed-page.ui", RESOURCE_PATH));

    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, stack);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, updates_banner);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, global_toggle);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, user_prefs_group);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, system_prefs_group);
    gtk_widget_class_bind_template_child (widget_class, ExmInstalledPage, search_prefs_group);

    gtk_widget_class_bind_template_callback (widget_class, on_bind_manager);
    gtk_widget_class_bind_template_callback (widget_class, on_visible_stack_changed);

    gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
exm_installed_page_init (ExmInstalledPage *self)
{
    GSettings *settings;
    GSimpleActionGroup *action_group;
    GSimpleAction *action;

    gtk_widget_init_template (GTK_WIDGET (self));

    settings = g_settings_new (APP_ID);

    g_settings_bind (settings, "sort-enabled-first",
                     self, "sort-enabled-first",
                     G_SETTINGS_BIND_GET);

    g_object_unref (settings);

    action_group = g_simple_action_group_new ();
    action = g_simple_action_new ("selection-changed", NULL);
    g_signal_connect (action, "activate",
                      G_CALLBACK (on_page_selection_changed), self);
    g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (action));
    gtk_widget_insert_action_group (GTK_WIDGET (self), "page",
                                    G_ACTION_GROUP (action_group));
    g_object_unref (action_group);

    g_signal_connect (self,
                      "notify::search-query",
                      G_CALLBACK (on_search_changed),
                      NULL);
}
