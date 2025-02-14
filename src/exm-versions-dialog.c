/*
 * exm-versions-dialog.c
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

#include "exm-versions-dialog.h"

#include "exm-config.h"
#include "web/model/exm-shell-versions.h"

#include <glib/gi18n.h>

struct _ExmVersionsDialog
{
    AdwDialog     parent_instance;

    GtkListBox   *version_list;
    GtkStack     *stack;
    GtkListBox   *more_list;
    AdwButtonRow *more_btn;
    gint          current_key;
};

G_DEFINE_FINAL_TYPE (ExmVersionsDialog, exm_versions_dialog, ADW_TYPE_DIALOG)

enum {
    SIGNAL_LOAD_MORE,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

/* Version keys: major * 1000 + minor.
 * GNOME 40+ collapses to minor = 0 (e.g. GNOME 46 → key 46000).
 * GNOME 3.x keeps its minor    (e.g. GNOME 3.38 → key 3038).
 * This gives correct ascending order across both eras. */

static void
normalize (gint *major,
           gint *minor)
{
    if (*major >= 40)
        *minor = 0;
}

static gint
version_key (gint major,
             gint minor)
{
    return major * 1000 + minor;
}

static gchar *
format_version (gint major,
                gint minor)
{
    return major >= 40
        ? g_strdup_printf ("%d", major)
        : g_strdup_printf ("%d.%d", major, minor);
}

static gchar *
format_date (const gchar *created)
{
    GDateTime *dt = NULL;
    gchar     *result;

    if (!created)
        return NULL;

    dt = g_date_time_new_from_iso8601 (created, g_time_zone_new_utc ());

    if (!dt && strlen (created) >= 10)
    {
        gchar *padded = g_strdup_printf ("%.10sT00:00:00Z", created);
        dt = g_date_time_new_from_iso8601 (padded, g_time_zone_new_utc ());
        g_free (padded);
    }

    if (!dt)
        return NULL;

    result = g_date_time_format (dt, "%x");
    g_date_time_unref (dt);
    return result;
}

static gint
sort_version_list (GtkListBoxRow *row_a,
                   GtkListBoxRow *row_b,
                   gpointer       user_data G_GNUC_UNUSED)
{
    gint v_a = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (row_a), "version"));
    gint v_b = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (row_b), "version"));

    return v_b - v_a;  /* descending: newest first */
}

static void
on_more_btn_activated (AdwButtonRow      *btn G_GNUC_UNUSED,
                       ExmVersionsDialog *self)
{
    gtk_widget_set_sensitive (GTK_WIDGET (self->more_btn), FALSE);
    g_signal_emit (self, signals[SIGNAL_LOAD_MORE], 0);
}

ExmVersionsDialog *
exm_versions_dialog_new ()
{
    return g_object_new (EXM_TYPE_VERSIONS_DIALOG, NULL);
}

void
exm_versions_dialog_set_current_version (ExmVersionsDialog *self,
                                         const gchar       *shell_version)
{
    gchar **parts;
    gint    major, minor;

    if (!shell_version)
        return;

    parts = g_strsplit (shell_version, ".", 2);
    if (!parts || !parts[0])
    {
        g_strfreev (parts);
        return;
    }

    major = atoi (parts[0]);
    minor = parts[1] ? atoi (parts[1]) : 0;
    g_strfreev (parts);

    normalize (&major, &minor);
    self->current_key = version_key (major, minor);
}

void
exm_versions_dialog_add_release (ExmVersionsDialog *self,
                                  gint               version,
                                  const gchar       *version_name,
                                  const gchar       *created,
                                  ExmShellVersions  *shell_versions)
{
    AdwActionRow *row;
    AdwWrapBox   *wrap_box;
    GHashTable   *seen;
    gchar        *title;
    gchar        *subtitle;

    gboolean has_name = version_name && *version_name;
    gchar   *date     = format_date (created);

    title = has_name ? g_strdup (version_name) : g_strdup_printf ("%d", version);

    if (has_name && date)
        subtitle = g_strdup_printf ("v%d · %s", version, date);
    else if (has_name)
        subtitle = g_strdup_printf ("v%d", version);
    else
        subtitle = g_steal_pointer (&date);

    g_free (date);

    row = ADW_ACTION_ROW (adw_action_row_new ());
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title);
    g_free (title);

    if (subtitle)
    {
        adw_action_row_set_subtitle (row, subtitle);
        g_free (subtitle);
    }

    wrap_box = ADW_WRAP_BOX (adw_wrap_box_new ());
    adw_wrap_box_set_child_spacing (wrap_box, 4);
    adw_wrap_box_set_line_spacing (wrap_box, 4);
    gtk_widget_set_valign (GTK_WIDGET (wrap_box), GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start (GTK_WIDGET (wrap_box), 8);

    seen = g_hash_table_new (g_direct_hash, g_direct_equal);

    if (shell_versions)
    {
        for (GList *l = shell_versions->versions; l != NULL; l = l->next)
        {
            Entry     *entry = l->data;
            gint       maj = entry->major;
            gint       min = entry->minor;
            gint       key;
            gchar     *label_text;
            GtkWidget *label;

            normalize (&maj, &min);
            key = version_key (maj, min);

            if (g_hash_table_contains (seen, GINT_TO_POINTER (key)))
                continue;

            g_hash_table_add (seen, GINT_TO_POINTER (key));

            label_text = format_version (maj, min);
            label = gtk_label_new (label_text);
            g_free (label_text);

            gtk_widget_add_css_class (label, "version-label");
            if (key == self->current_key)
                gtk_widget_add_css_class (label, "current");

            adw_wrap_box_append (wrap_box, label);
        }
    }

    g_hash_table_unref (seen);

    adw_action_row_add_suffix (row, GTK_WIDGET (wrap_box));

    g_object_set_data (G_OBJECT (row), "version", GINT_TO_POINTER (version));

    gtk_list_box_append (self->version_list, GTK_WIDGET (row));
    gtk_stack_set_visible_child_name (self->stack, "list");
}

void
exm_versions_dialog_set_compatible_release (ExmVersionsDialog *self,
                                             gint               version)
{
    GtkListBoxRow *row;

    for (gint i = 0; (row = gtk_list_box_get_row_at_index (self->version_list, i)) != NULL; i++)
    {
        if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (row), "version")) == version)
        {
            GtkWidget *icon = gtk_image_new_from_icon_name ("object-select-symbolic");
            gtk_widget_set_valign (icon, GTK_ALIGN_CENTER);
            gtk_widget_set_tooltip_text (icon, _("Compatible with your system"));
            adw_action_row_add_suffix (ADW_ACTION_ROW (row), icon);
            break;
        }
    }
}

void
exm_versions_dialog_finish (ExmVersionsDialog *self,
                             gboolean           has_more)
{
    if (g_strcmp0 (gtk_stack_get_visible_child_name (self->stack), "list") != 0)
        gtk_stack_set_visible_child_name (self->stack, "empty");

    gtk_widget_set_visible (GTK_WIDGET (self->more_list), has_more);
    gtk_widget_set_sensitive (GTK_WIDGET (self->more_btn), TRUE);
}

static void
exm_versions_dialog_class_init (ExmVersionsDialogClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class,
        g_strdup_printf ("%s/exm-versions-dialog.ui", RESOURCE_PATH));

    gtk_widget_class_bind_template_child (widget_class, ExmVersionsDialog, version_list);
    gtk_widget_class_bind_template_child (widget_class, ExmVersionsDialog, stack);
    gtk_widget_class_bind_template_child (widget_class, ExmVersionsDialog, more_list);
    gtk_widget_class_bind_template_child (widget_class, ExmVersionsDialog, more_btn);

    gtk_widget_class_bind_template_callback (widget_class, on_more_btn_activated);

    signals[SIGNAL_LOAD_MORE] = g_signal_new ("load-more",
                                               G_TYPE_FROM_CLASS (klass),
                                               G_SIGNAL_RUN_LAST,
                                               0, NULL, NULL, NULL,
                                               G_TYPE_NONE, 0);
}

static void
exm_versions_dialog_init (ExmVersionsDialog *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    self->current_key = -1;

    gtk_list_box_set_sort_func (self->version_list, sort_version_list, NULL, NULL);
}
