/*
 * exm-upgrade-assistant.c
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

#include "exm-upgrade-assistant.h"

#include "web/exm-versions-provider.h"
#include "exm-upgrade-result.h"

#include "exm-config.h"

#include <glib/gi18n.h>

struct _ExmUpgradeAssistant
{
    AdwDialog parent_instance;

    // Auxiliary Classes
    ExmManager *manager;
    ExmVersionsProvider *versions_provider;

    GCancellable *cancellable;

    // Version Data
    gchar *target_shell_version;

    // Results Data
    gchar *current_shell_version;
    int total_extensions;
    int number_checked;
    int number_supported;
    gboolean waiting_on_tasks;
    GListStore *user_results_store;
    GListStore *system_results_store;

    // Template Widgets
    AdwToastOverlay *toast_overlay;
    AdwNavigationView *navigation_view;
    GtkStack *stack;

    // Version Select Page
    GtkLabel *description;
    AdwComboRow *drop_down;
    GtkButton *run_button;

    // Waiting Page
    AdwStatusPage *counter;

    // Error Page
    AdwStatusPage *error_status;

    // Results Page
    AdwPreferencesPage *prefs_page;
    GtkLabel *summary_top;
    GtkProgressBar *progress_bar;
    GtkLabel *summary_bottom;
    AdwPreferencesGroup *user_prefs_group;
    AdwPreferencesGroup *system_prefs_group;
    AdwButtonRow *copy_details;
};

G_DEFINE_FINAL_TYPE (ExmUpgradeAssistant, exm_upgrade_assistant, ADW_TYPE_DIALOG)

enum {
    PROP_0,
    PROP_MANAGER,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmUpgradeAssistant *
exm_upgrade_assistant_new (ExmManager *manager)
{
    return g_object_new (EXM_TYPE_UPGRADE_ASSISTANT,
                         "manager", manager,
                         NULL);
}

static void
exm_upgrade_assistant_dispose (GObject *object)
{
    ExmUpgradeAssistant *self = (ExmUpgradeAssistant *)object;

    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);

    G_OBJECT_CLASS (exm_upgrade_assistant_parent_class)->dispose (object);
}

static void
exm_upgrade_assistant_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
    ExmUpgradeAssistant *self = EXM_UPGRADE_ASSISTANT (object);

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
exm_upgrade_assistant_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
    ExmUpgradeAssistant *self = EXM_UPGRADE_ASSISTANT (object);

    switch (prop_id)
    {
    case PROP_MANAGER:
        self->manager = g_value_get_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

typedef struct
{
    ExmExtension *local_data;
    ExmUpgradeAssistant *assistant;
    gboolean is_user;
} ExtensionCheckData;

typedef enum
{
    STATUS_SUPPORTED,
    STATUS_UNSUPPORTED,
    STATUS_UNKNOWN,
} SupportStatus;

static ExtensionCheckData *
create_check_data (ExmExtension        *local_data,
                   ExmUpgradeAssistant *assistant,
                   gboolean             is_user)
{
    ExtensionCheckData* data;

    data = g_slice_new0 (ExtensionCheckData);
    data->local_data = g_object_ref (local_data);
    data->assistant = g_object_ref (assistant);
    data->is_user = is_user;

    return data;
}

static void
free_check_data (ExtensionCheckData *data)
{
    if (data)
    {
        g_clear_object (&data->local_data);
        g_clear_object (&data->assistant);
        g_slice_free (ExtensionCheckData, data);
    }
}

static void
update_checked_count (ExmUpgradeAssistant *self)
{
    char *text;

    text = g_strdup_printf (_("Checked %d/%d Extensions"),
                            self->number_checked,
                            self->total_extensions);

    adw_status_page_set_title (self->counter, text);
    g_free (text);
}

static void
display_results (ExmUpgradeAssistant *self)
{
    char *text;
    float fraction;

    // Set progress bar value
    fraction = (float)self->number_supported / (float)self->total_extensions;
    gtk_progress_bar_set_fraction (self->progress_bar, fraction);

    // Set percentage text
    text = g_strdup_printf (_("%d%% Compatible"), (int)(fraction * 100));
    gtk_progress_bar_set_text (self->progress_bar, text);
    free (text);

    // Colour according to percentage
    gtk_widget_remove_css_class (GTK_WIDGET (self->progress_bar), "success");
    gtk_widget_remove_css_class (GTK_WIDGET (self->progress_bar), "warning");
    gtk_widget_remove_css_class (GTK_WIDGET (self->progress_bar), "error");

    if (fraction == 1.0f)
        gtk_widget_add_css_class (GTK_WIDGET (self->progress_bar), "success");
    else if (fraction <= 1.0f && fraction > 0.3f)
        gtk_widget_add_css_class (GTK_WIDGET (self->progress_bar), "warning");
    else if (fraction <= 0.3f)
        gtk_widget_add_css_class (GTK_WIDGET (self->progress_bar), "error");

    text = _("<b>%d out of %d</b> extensions currently installed on this system have been marked as supporting <b>GNOME %s</b>");
    text = g_strdup_printf (text, self->number_supported, self->total_extensions, self->target_shell_version);
    gtk_label_set_markup (self->summary_top, text);
    g_free (text);

    if (self->total_extensions - self->number_supported == 0)
    {
        text = _("If you switch to GNOME %s now, all of your extensions should work");
        text = g_strdup_printf (text, self->target_shell_version);
    }
    else
    {
        text = _("If you switch to GNOME %s now, %d of your extensions might not work");
        text = g_strdup_printf (text, self->target_shell_version, self->total_extensions - self->number_supported);
    }

    gtk_label_set_markup (self->summary_bottom, text);
    g_free (text);

    adw_preferences_page_scroll_to_top (self->prefs_page);
    gtk_stack_set_visible_child_name (self->stack, "results");
}

static SupportStatus
get_support_status (ExmUpgradeResult *result,
                    const char       *target_version)
{
    SupportStatus supported;
    ExmVersionResult *web_data;

    web_data = exm_upgrade_result_get_web_data (result);

    if (web_data && exm_version_result_supports_shell_version (web_data, target_version))
        supported = STATUS_SUPPORTED;
    else if (web_data)
        supported = STATUS_UNSUPPORTED;
    else
        supported = STATUS_UNKNOWN;

    return supported;
}

static void
print_list_model (GListModel *model,
                  GString    *string_builder,
                  gchar      *target_shell_version)
{
    int num_extensions;
    gchar *text;
    int i;

    num_extensions = g_list_model_get_n_items (model);
    for (i = 0; i < num_extensions; i++)
    {
        ExmUpgradeResult *result;
        const gchar *name, *uuid, *supported_text;
        SupportStatus supported;

        result = g_list_model_get_item (model, i);

        name = exm_upgrade_result_get_name (result);
        uuid = exm_upgrade_result_get_uuid (result);

        supported = get_support_status (result, target_shell_version);

        text = g_strdup_printf ("%s\n", name);
        g_string_append (string_builder, text);
        g_free (text);

        text = g_strdup_printf ("Extension ID: %s\n", uuid);
        g_string_append (string_builder, text);
        g_free (text);

        if (supported == STATUS_SUPPORTED)
            supported_text = "Yes";
        else if (supported == STATUS_UNSUPPORTED)
            supported_text = "No";
        else
            supported_text = "Unknown";

        text = g_strdup_printf ("Supported: %s\n\n", supported_text);
        g_string_append (string_builder, text);
        g_free (text);
    }
}

static void
copy_to_clipboard (ExmUpgradeAssistant *self)
{
    GString *string_builder;
    gchar *text;
    float fraction;

    GdkDisplay *display;
    GdkClipboard *clipboard;

    fraction = (float)self->number_supported / (float)self->total_extensions;

    string_builder = g_string_new ("Extension Manager - Upgrade Assistant Report\n\n");

    text = g_strdup_printf ("Currently on: GNOME %s\n", self->current_shell_version);
    g_string_append (string_builder, text);
    g_free (text);

    text = g_strdup_printf ("Upgrading to: GNOME %s\n\n", self->target_shell_version);
    g_string_append (string_builder, text);
    g_free (text);

    text = g_strdup_printf ("On upgrading to GNOME %s, %d out of %d currently\ninstalled extensions will be compatible (%d%%).\n\n",
                            self->target_shell_version,
                            self->number_supported,
                            self->total_extensions,
                            (int)(fraction * 100));
    g_string_append (string_builder, text);
    g_free (text);

    if (g_list_model_get_n_items (G_LIST_MODEL (self->user_results_store)) > 0)
    {
        text = g_strdup_printf ("User-Installed Extensions:\n\n");
        g_string_append (string_builder, text);
        g_free (text);

        print_list_model (G_LIST_MODEL (self->user_results_store), string_builder, self->target_shell_version);
    }

    if (g_list_model_get_n_items (G_LIST_MODEL (self->system_results_store)) > 0)
    {
        text = g_strdup_printf ("\nSystem Extensions:\n\n");
        g_string_append (string_builder, text);
        g_free (text);

        print_list_model (G_LIST_MODEL (self->system_results_store), string_builder, self->target_shell_version);
    }

    // Add to clipboard
    display = gdk_display_get_default ();
    clipboard = gdk_display_get_clipboard (display);

    text = g_string_free (string_builder, FALSE);
    gdk_clipboard_set_text (clipboard, text);
    g_free (text);

    // Success indicator
    adw_toast_overlay_add_toast (self->toast_overlay, adw_toast_new (_("Copied")));
}

static void
display_extension_result (ExmUpgradeAssistant *self,
                          ExmUpgradeResult    *result,
                          gboolean             is_user)
{
    if (get_support_status (result, self->target_shell_version) == STATUS_SUPPORTED)
        self->number_supported++;

    g_list_store_append (is_user ? self->user_results_store : self->system_results_store, result);

    if (self->waiting_on_tasks && self->number_checked == self->total_extensions)
        display_results (self);
}

static void
on_extension_processed (GObject      *source,
                        GAsyncResult *async_result,
                        gpointer      user_data)
{
    GListModel *list;
    GError *error = NULL;
    ExtensionCheckData *data;
    ExmUpgradeAssistant *self;
    gchar *next = NULL;
    ExmVersionResult *web_version = NULL;
    ExmVersionResult *compatible_version = NULL;

    g_return_if_fail (user_data != NULL);

    list = exm_versions_provider_query_finish (EXM_VERSIONS_PROVIDER (source), async_result, &next, &error);
    data = (ExtensionCheckData *) user_data;
    self = EXM_UPGRADE_ASSISTANT (data->assistant);

    if (error &&
        (error->domain != g_quark_try_string ("request-error-quark") || error->code != 404))
    {
        adw_status_page_set_description (self->error_status, error->message);
        gtk_stack_set_visible_child_name (self->stack, "error");

        g_clear_error (&error);
        free_check_data (data);
        g_free (next);
        return;
    }

    for (guint i = 0; i < g_list_model_get_n_items (list); i++)
    {
        ExmVersionResult *item = EXM_VERSION_RESULT (g_list_model_get_object (list, i));

        if (!web_version)
            web_version = g_object_ref (item);

        if (!compatible_version &&
            exm_version_result_supports_shell_version (item, self->target_shell_version))
            compatible_version = g_object_ref (item);

        g_object_unref (item);
    }

    g_object_unref (list);

    if (!compatible_version && next != NULL)
    {
        exm_versions_provider_query_next_async (self->versions_provider,
                                                next,
                                                self->cancellable,
                                                on_extension_processed,
                                                data);
        g_free (next);
        return;
    }

    g_free (next);

    self->number_checked++;
    update_checked_count (self);

    ExmUpgradeResult *result = exm_upgrade_result_new ();
    exm_upgrade_result_set_local_data (result, data->local_data);

    if (compatible_version)
        exm_upgrade_result_set_web_data (result, compatible_version);
    else if (web_version)
        exm_upgrade_result_set_web_data (result, web_version);

    display_extension_result (self, result, data->is_user);

    g_clear_object (&compatible_version);
    g_clear_object (&web_version);
    free_check_data (data);
}

static void
do_compatibility_check (ExmUpgradeAssistant *self)
{
    GListModel *ext_model;
    int num_items;
    int i;

    gint selected;
    GListModel *model;
    const char *target_version;

    selected = adw_combo_row_get_selected (self->drop_down);
    model = adw_combo_row_get_model (self->drop_down);
    target_version = gtk_string_list_get_string (GTK_STRING_LIST (model), selected);

    if (!target_version)
        return;

    if (self->target_shell_version)
        g_free (self->target_shell_version);

    self->target_shell_version = g_strdup_printf ("%s", target_version);

    if (!self->manager)
        return;

    g_object_get (self->manager,
                  "extensions", &ext_model,
                  NULL);

    num_items = g_list_model_get_n_items (ext_model);

    if (num_items == 0)
    {
        adw_toast_overlay_add_toast (self->toast_overlay, adw_toast_new (_("No Extensions Installed")));
        return;
    }

    // Display spinner
    gtk_stack_set_visible_child_name (self->stack, "waiting");
    adw_navigation_view_push_by_tag (self->navigation_view, "results");

    // Reset variables
    self->total_extensions = 0;
    self->number_checked = 0;
    self->number_supported = 0;
    self->waiting_on_tasks = FALSE;
    update_checked_count (self);

    // Empty results list stores before processing items
    g_list_store_remove_all (self->user_results_store);
    g_list_store_remove_all (self->system_results_store);

    for (i = 0; i < num_items; i++)
    {
        char *uuid;
        gboolean is_user;
        ExmExtension *extension;
        ExtensionCheckData *data;

        extension = EXM_EXTENSION (g_list_model_get_item (ext_model, i));

        g_object_get (extension,
                      "uuid", &uuid,
                      "is-user", &is_user,
                      NULL);
        g_debug ("Processing: %s\n", uuid);

        data = create_check_data (extension, self, is_user);

        self->total_extensions++;
        exm_versions_provider_query_async (self->versions_provider, uuid, self->cancellable,
                                           on_extension_processed, data);
    }

    // Set this flag after all tasks have been dispatched
    // so we do not accidentally finish too early.
    self->waiting_on_tasks = TRUE;
}

static GtkWidget *
widget_factory (ExmUpgradeResult    *result,
                ExmUpgradeAssistant *self)
{
    SupportStatus supported;
    const gchar *name, *uuid;
    GtkWidget *row, *status;

    g_return_val_if_fail (EXM_IS_UPGRADE_RESULT (result), NULL);

    name = exm_upgrade_result_get_name (result);
    uuid = exm_upgrade_result_get_uuid (result);

    supported = get_support_status (result, self->target_shell_version);

    row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row) , name);
    adw_action_row_set_subtitle (ADW_ACTION_ROW (row), uuid);

    if (supported == STATUS_SUPPORTED)
    {
        status = gtk_image_new_from_icon_name ("supported-symbolic");
        gtk_widget_add_css_class (status, "success");
        // Translators: Icon's tooltip when an extension is compatible
        gtk_widget_set_tooltip_text (GTK_WIDGET (status), _("A compatible version of the extension exists"));
    }
    else if (supported == STATUS_UNSUPPORTED)
    {
        status = gtk_image_new_from_icon_name ("unsupported-symbolic");
        gtk_widget_add_css_class (status, "error");
        // Translators: Icon's tooltip when an extension is not compatible
        gtk_widget_set_tooltip_text (GTK_WIDGET (status), _("No compatible version of the extension exists"));
    }
    else
    {
        status = gtk_image_new_from_icon_name ("unknown-symbolic");
        gtk_widget_add_css_class (status, "warning");
        // Translators: Icon's tooltip when an extension compatibility is unknown
        gtk_widget_set_tooltip_text (GTK_WIDGET (status), _("This extension is not hosted on extensions.gnome.org. Its compatibility cannot be determined."));
    }

    gtk_image_set_pixel_size (GTK_IMAGE (status), 24);
    gtk_widget_set_focusable (status, TRUE);
    adw_action_row_add_suffix (ADW_ACTION_ROW (row), status);

    return row;
}

static void
bind_prefs_group (ExmUpgradeAssistant *self,
                  AdwPreferencesGroup *prefs_group,
                  GListModel          *model)
{
    GtkExpression *expression;
    GtkStringSorter *alphabetical_sorter;
    GtkSortListModel *sorted_model;

    g_return_if_fail (ADW_IS_PREFERENCES_GROUP (prefs_group));
    g_return_if_fail (G_IS_LIST_MODEL (model));

    // Sort alphabetically
    expression = gtk_property_expression_new (EXM_TYPE_UPGRADE_RESULT, NULL, "name");
    alphabetical_sorter = gtk_string_sorter_new (expression);

    sorted_model = gtk_sort_list_model_new (model, GTK_SORTER (alphabetical_sorter));

    adw_preferences_group_bind_model (prefs_group, G_LIST_MODEL (sorted_model),
                                      (GtkListBoxCreateWidgetFunc) widget_factory,
                                      self, NULL);
}

static void
populate_drop_down (ExmUpgradeAssistant *self)
{
    GDateTime *date_time;
    GtkStringList *string_list;
    int year, month;
    int current_gnome_version;
    int index;

    date_time = g_date_time_new_now_utc ();
    year = g_date_time_get_year (date_time);
    month = g_date_time_get_month (date_time);

    // Two GNOME versions are released per year, once in march
    // and once in september. Guess/approximate the newest release
    // of GNOME and make that the default.

    // GNOME 40 came out in March 2021
    current_gnome_version = 40 + (year - 2021) * 2;

    // If we are between march and september, then it is
    // an even numbered release, otherwise use an odd numbered
    // release.
    if (month < G_DATE_MARCH)
        current_gnome_version -= 1;
    else if (month >= G_DATE_SEPTEMBER)
        current_gnome_version += 1;

    g_info ("Current GNOME Version: %d\n", current_gnome_version);

    // Make sure we at least have GNOME 40-43 regardless
    // of the date calculation. Update this periodically
    current_gnome_version = MAX(43, current_gnome_version);

    // Populate dropdown and version map
    string_list = gtk_string_list_new (NULL);

    for (index = 40; index <= current_gnome_version; index++)
    {
        gchar *key = g_strdup_printf ("%d", index);
        gtk_string_list_append (string_list, key);
        g_free (key);
    }

    adw_combo_row_set_model (self->drop_down, G_LIST_MODEL (string_list));
    adw_combo_row_set_selected (self->drop_down, g_list_model_get_n_items (G_LIST_MODEL (string_list)) - 1);
}

static void
on_bind_manager (ExmUpgradeAssistant *self)
{
    gchar *description_text;

    g_object_get (self->manager,
                  "shell-version",
                  &self->current_shell_version,
                  NULL);

    description_text = _("You are currently running <b>GNOME %s</b>. Select a version of GNOME below and check whether your extensions will continue to be available.");
    description_text = g_strdup_printf (description_text, self->current_shell_version);

    gtk_label_set_markup (self->description, description_text);

    g_free (description_text);
}

static void
on_hidden_results (AdwNavigationPage   *page G_GNUC_UNUSED,
                   ExmUpgradeAssistant *self)
{
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
    self->cancellable = g_cancellable_new ();
}

static void
exm_upgrade_assistant_class_init (ExmUpgradeAssistantClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = exm_upgrade_assistant_dispose;
    object_class->get_property = exm_upgrade_assistant_get_property;
    object_class->set_property = exm_upgrade_assistant_set_property;

    properties [PROP_MANAGER]
        = g_param_spec_object ("manager",
                               "Manager",
                               "Manager",
                               EXM_TYPE_MANAGER,
                               G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-upgrade-assistant.ui", RESOURCE_PATH));
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, toast_overlay);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, navigation_view);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, stack);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, description);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, drop_down);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, run_button);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, counter);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, error_status);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, prefs_page);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, summary_top);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, progress_bar);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, summary_bottom);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, user_prefs_group);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, system_prefs_group);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, copy_details);

    gtk_widget_class_bind_template_callback (widget_class, on_bind_manager);
    gtk_widget_class_bind_template_callback (widget_class, do_compatibility_check);
    gtk_widget_class_bind_template_callback (widget_class, on_hidden_results);
    gtk_widget_class_bind_template_callback (widget_class, copy_to_clipboard);
}

static void
exm_upgrade_assistant_init (ExmUpgradeAssistant *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    self->versions_provider = exm_versions_provider_new ();
    self->target_shell_version = NULL;
    self->cancellable = g_cancellable_new ();

    self->user_results_store = g_list_store_new (EXM_TYPE_UPGRADE_RESULT);
    self->system_results_store = g_list_store_new (EXM_TYPE_UPGRADE_RESULT);
    bind_prefs_group (self, self->user_prefs_group, G_LIST_MODEL (self->user_results_store));
    bind_prefs_group (self, self->system_prefs_group, G_LIST_MODEL (self->system_results_store));

    populate_drop_down (self);

    g_object_bind_property (self->user_results_store, "n-items",
                            self->user_prefs_group, "visible",
                            G_BINDING_SYNC_CREATE);

    g_object_bind_property (self->system_results_store, "n-items",
                            self->system_prefs_group, "visible",
                            G_BINDING_SYNC_CREATE);
}
