/*
 * exm-upgrade-assistant.c
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

#include "exm-upgrade-assistant.h"

#include "web/exm-data-provider.h"

#include <glib/gi18n.h>

struct _ExmUpgradeAssistant
{
    AdwWindow parent_instance;

    // Auxiliary Classes
    ExmManager *manager;
    ExmDataProvider *data_provider;

    // Version Data
    gchar *target_shell_version;
    GHashTable *version_map;

    // Async Tracking
    gchar *current_shell_version;
    int total_extensions;
    int number_checked;
    int number_supported;
    gboolean waiting_on_tasks;

    // Template Widgets
    GtkStack *stack;

    // Version Select Page
    GtkDropDown *drop_down;
    GtkButton *run_button;
    GtkLabel *description;

    // Waiting Page
    GtkLabel *counter;

    // Results Page
    GtkListView *list_view;
    GtkProgressBar *progress_bar;
    GtkLabel *summary;
};

G_DEFINE_FINAL_TYPE (ExmUpgradeAssistant, exm_upgrade_assistant, ADW_TYPE_WINDOW)

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
exm_upgrade_assistant_finalize (GObject *object)
{
    ExmUpgradeAssistant *self = (ExmUpgradeAssistant *)object;

    G_OBJECT_CLASS (exm_upgrade_assistant_parent_class)->finalize (object);
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

static void
update_checked_count (ExmUpgradeAssistant *self)
{
    char *text;

    text = g_strdup_printf ("Checked %d/%d extensions",
                            self->number_checked,
                            self->total_extensions);

    gtk_label_set_text (self->counter, text);
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
    text = g_strdup_printf ("%d%% Compatible", (int)(fraction * 100));
    gtk_progress_bar_set_text (self->progress_bar, text);
    free (text);

    // Colour according to percentage
    gtk_widget_remove_css_class (GTK_WIDGET (self->progress_bar), "success");
    gtk_widget_remove_css_class (GTK_WIDGET (self->progress_bar), "warning");
    gtk_widget_remove_css_class (GTK_WIDGET (self->progress_bar), "error");

    if (fraction == 1.0f) {
        // make green
        gtk_widget_add_css_class (GTK_WIDGET (self->progress_bar), "success");
    } else if (fraction <= 0.7f && fraction > 0.3f) {
        // make orange
        gtk_widget_add_css_class (GTK_WIDGET (self->progress_bar), "warning");
    } else if (fraction <= 0.3f) {
        // make red
        gtk_widget_add_css_class (GTK_WIDGET (self->progress_bar), "error");
    }

    text = _("<b>GNOME %s</b> supports <b>%d out of %d</b> of the extensions currently installed on the system.");
    text = g_strdup_printf (text, self->target_shell_version, self->number_supported, self->total_extensions);
    gtk_label_set_markup (self->summary, text);
    g_free (text);

    gtk_stack_set_visible_child_name (self->stack, "results");
}

static void
display_extension_result (ExmUpgradeAssistant *self,
                          ExmSearchResult     *extension,
                          gboolean             is_user)
{
    gint pk;
    gboolean is_supported;
    gchar *uuid, *name, *creator, *icon_uri, *screenshot_uri, *link, *description;
    g_object_get (extension,
                  "uuid", &uuid,
                  "name", &name,
                  "creator", &creator,
                  "icon", &icon_uri,
                  "screenshot", &screenshot_uri,
                  "link", &link,
                  "description", &description,
                  "pk", &pk,
                  NULL);

    is_supported = exm_search_result_supports_shell_version (extension, self->target_shell_version);

    if (is_supported) {
        self->number_supported++;
    }

    g_print ("Extension '%s' is supported on GNOME %s: %d\n", name, self->target_shell_version, is_supported);

    if (self->waiting_on_tasks && self->number_checked == self->total_extensions) {
        display_results (self);
    }
}

static void
on_user_ext_processed (GObject      *source,
                       GAsyncResult *result,
                       gpointer      user_data)
{
    ExmSearchResult *data;
    GError *error = NULL;
    ExmUpgradeAssistant *self;

    self = EXM_UPGRADE_ASSISTANT (user_data);

    self->number_checked++;
    update_checked_count (self);

    if ((data = exm_data_provider_get_finish (EXM_DATA_PROVIDER (source), result, &error)) != FALSE)
    {
        display_extension_result (self, data, TRUE);
    }
}

static void
on_system_ext_processed (GObject      *source,
                         GAsyncResult *result,
                         gpointer      user_data)
{
    ExmSearchResult *data;
    GError *error = NULL;
    ExmUpgradeAssistant *self;

    self = EXM_UPGRADE_ASSISTANT (user_data);

    self->number_checked++;
    update_checked_count (self);

    if ((data = exm_data_provider_get_finish (EXM_DATA_PROVIDER (source), result, &error)) != FALSE)
    {
        display_extension_result (self, data, FALSE);
    }
}

static void
do_compatibility_check (ExmUpgradeAssistant *self)
{
    GListModel *user_ext_model;
    GListModel *system_ext_model;
    int num_items;
    int i;

    int selected;
    const char *key;
    GListModel *model;
    int target_version;

    selected = gtk_drop_down_get_selected (self->drop_down);
    model = gtk_drop_down_get_model (self->drop_down);
    key = gtk_string_list_get_string (GTK_STRING_LIST (model), selected);

    if (!key)
        return;

    target_version = (int) g_hash_table_lookup (self->version_map, key);

    if (self->target_shell_version)
        g_free (self->target_shell_version);

    self->target_shell_version = g_strdup_printf ("%d", target_version);

    if (!self->manager)
        return;

    g_object_get (self->manager,
                  "user-extensions", &user_ext_model,
                  "system-extensions", &system_ext_model,
                  NULL);

    gtk_stack_set_visible_child_name (self->stack, "waiting");
    self->total_extensions = 0;
    self->number_checked = 0;
    self->number_supported = 0;
    self->waiting_on_tasks = FALSE;
    update_checked_count (self);

    num_items = g_list_model_get_n_items (user_ext_model);
    for (i = 0; i < num_items; i++) {
        char *uuid;
        ExmExtension *extension;

        extension = EXM_EXTENSION (g_list_model_get_item (user_ext_model, i));

        g_object_get (extension, "uuid", &uuid, NULL);
        g_print ("Processing: %s\n", uuid);

        self->total_extensions++;
        exm_data_provider_get_async (self->data_provider, uuid, NULL, on_user_ext_processed, self);
    }

    num_items = g_list_model_get_n_items (system_ext_model);
    for (i = 0; i < num_items; i++) {
        char *uuid;
        ExmExtension *extension;

        extension = EXM_EXTENSION (g_list_model_get_item (system_ext_model, i));

        g_object_get (extension, "uuid", &uuid, NULL);
        g_print ("Processing: %s\n", uuid);

        self->total_extensions++;
        exm_data_provider_get_async (self->data_provider, uuid, NULL, on_system_ext_processed, self);
    }

    // Set this flag after all tasks have been dispatched
    // so we do not accidentally finish too early.
    self->waiting_on_tasks = TRUE;
}

static void
populate_drop_down (ExmUpgradeAssistant *self)
{
    GDateTime *date_time;
    GtkStringList *string_list;
    GHashTable *hash_table;
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

    // If we are between september and march, then it is
    // an odd numbered release, otherwise use an even numbered
    // release.
    if (month >= G_DATE_SEPTEMBER || month < G_DATE_MARCH)
        current_gnome_version += 1;

    g_print ("Current GNOME Version: %d\n", current_gnome_version);

    // Make sure we at least have GNOME 40-43 regardless
    // of the date calculation. Update this periodically
    current_gnome_version = MAX(43, current_gnome_version);

    // Populate dropdown and version map
    string_list = gtk_string_list_new (NULL);
    hash_table = g_hash_table_new (g_str_hash, g_str_equal);

    for (index = 40; index <= current_gnome_version; index++) {
        gchar *key;

        key = g_strdup_printf ("GNOME %d", index);
        g_hash_table_insert (hash_table, key, index);
        gtk_string_list_append (string_list, key);
    }

    self->version_map = hash_table;
    gtk_drop_down_set_model (self->drop_down, G_LIST_MODEL (string_list));
    gtk_drop_down_set_selected (self->drop_down, g_list_model_get_n_items (G_LIST_MODEL (string_list)) - 1);
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
exm_upgrade_assistant_class_init (ExmUpgradeAssistantClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_upgrade_assistant_finalize;
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

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-upgrade-assistant.ui");
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, list_view);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, run_button);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, drop_down);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, description);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, stack);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, counter);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, progress_bar);
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, summary);
}

static void
exm_upgrade_assistant_init (ExmUpgradeAssistant *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    g_signal_connect_swapped (self->run_button,
                              "clicked",
                              G_CALLBACK (do_compatibility_check),
                              self);

    g_signal_connect (self,
                      "notify::manager",
                      G_CALLBACK (on_bind_manager),
                      NULL);

    self->data_provider = exm_data_provider_new ();
    self->target_shell_version = NULL;

    populate_drop_down (self);
}
