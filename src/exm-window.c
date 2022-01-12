/* exm-window.c
 *
 * Copyright 2022 Matthew Jakeman
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
 */

#include "exm-config.h"
#include "exm-window.h"

#include "model/exm-manager.h"
#include "model/exm-extension.h"

#include "web/exm-search-provider.h"
#include "web/exm-search-result.h"
#include "web/exm-image-resolver.h"

#include <adwaita.h>
#include <glib/gi18n.h>

struct _ExmWindow
{
    GtkApplicationWindow  parent_instance;

    ExmManager *manager;
    ExmSearchProvider *search;
    ExmImageResolver *resolver;

    GListModel *search_results_model;

    /* Template widgets */
    AdwHeaderBar        *header_bar;
    GtkListBox          *user_list_box;
    GtkListBox          *system_list_box;
    GtkSearchEntry      *search_entry;
    GtkListBox          *search_results;
};

G_DEFINE_TYPE (ExmWindow, exm_window, GTK_TYPE_APPLICATION_WINDOW)

static void
exm_window_class_init (ExmWindowClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-window.ui");
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, header_bar);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, user_list_box);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, system_list_box);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, search_entry);
    gtk_widget_class_bind_template_child (widget_class, ExmWindow, search_results);
}

static gboolean
extension_state_set (GtkSwitch    *toggle,
                     gboolean      state,
                     ExmExtension *extension)
{
    GtkRoot *root = gtk_widget_get_root (GTK_WIDGET (toggle));
    ExmWindow *self = EXM_WINDOW (root);

    if (state)
        exm_manager_enable_extension (self->manager, extension);
    else
        exm_manager_disable_extension (self->manager, extension);

    return FALSE;
}

static void
extension_open_prefs (GtkButton    *button,
                      ExmExtension *extension)
{
    GtkRoot *root = gtk_widget_get_root (GTK_WIDGET (button));
    ExmWindow *self = EXM_WINDOW (root);

    exm_manager_open_prefs (self->manager, extension);
}

typedef struct
{
    ExmManager *manager;
    ExmExtension *extension;
} RemoveDialogData;

static void
extension_remove_dialog_response (GtkDialog        *dialog,
                                  int               response_id,
                                  RemoveDialogData *data)
{
    gtk_window_destroy (GTK_WINDOW (dialog));

    if (response_id == GTK_RESPONSE_YES)
    {
        exm_manager_remove_extension (data->manager, data->extension);
    }

    g_clear_pointer (&data->manager, g_object_unref);
    g_clear_pointer (&data->extension, g_object_unref);
    g_free (data);
}

static void
extension_remove (GtkButton    *button,
                  ExmExtension *extension)
{
    GtkRoot *root = gtk_widget_get_root (GTK_WIDGET (button));
    ExmWindow *self = EXM_WINDOW (root);

    GtkWidget *dlg;

    dlg = gtk_message_dialog_new (GTK_WINDOW (self),
                                  GTK_DIALOG_MODAL,
                                  GTK_MESSAGE_QUESTION,
                                  GTK_BUTTONS_YES_NO,
                                  _("Are you sure you want to uninstall?"));

    RemoveDialogData *data = g_new0 (RemoveDialogData, 1);
    data->manager = g_object_ref (self->manager);
    data->extension = g_object_ref (extension);

    g_signal_connect (dlg, "response", G_CALLBACK (extension_remove_dialog_response), data);
    gtk_widget_show (dlg);
}

static GtkWidget *
widget_factory (ExmExtension* extension)
{
    GtkWidget *row;
    GtkWidget *label;
    GtkWidget *toggle;
    GtkWidget *prefs;
    GtkWidget *remove;

    gchar *name, *uuid, *description;
    gboolean enabled, has_prefs, is_user;
    g_object_get (extension,
                  "display-name", &name,
                  "uuid", &uuid,
                  "description", &description,
                  "enabled", &enabled,
                  "has-prefs", &has_prefs,
                  "is-user", &is_user,
                  NULL);

    name = g_markup_escape_text (name, -1);

    row = adw_expander_row_new ();

    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), name);
    adw_expander_row_set_subtitle (ADW_EXPANDER_ROW (row), uuid);

    toggle = gtk_switch_new ();
    gtk_switch_set_state (GTK_SWITCH (toggle), enabled);
    gtk_widget_set_valign (toggle, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (toggle, GTK_ALIGN_CENTER);
    adw_expander_row_add_action (ADW_EXPANDER_ROW (row), toggle);
    g_signal_connect (toggle, "state-set", G_CALLBACK (extension_state_set), extension);

    if (has_prefs)
    {
        prefs = gtk_button_new_from_icon_name ("settings-symbolic");
        gtk_widget_set_valign (prefs, GTK_ALIGN_CENTER);
        gtk_widget_set_halign (prefs, GTK_ALIGN_CENTER);
        g_signal_connect (prefs, "clicked", G_CALLBACK (extension_open_prefs), extension);
        adw_expander_row_add_action (ADW_EXPANDER_ROW (row), prefs);
    }

    label = gtk_label_new (description);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_label_set_wrap (GTK_LABEL (label), GTK_WRAP_WORD);
    gtk_widget_add_css_class (label, "content");
    adw_expander_row_add_row (ADW_EXPANDER_ROW (row), label);

    if (is_user)
    {
        remove = gtk_button_new_with_label (_("Remove"));
        gtk_widget_add_css_class (remove, "destructive-action");
        gtk_widget_set_valign (remove, GTK_ALIGN_CENTER);
        gtk_widget_set_halign (remove, GTK_ALIGN_END);
        g_signal_connect (remove, "clicked", G_CALLBACK (extension_remove), extension);
        adw_expander_row_add_row (ADW_EXPANDER_ROW (row), remove);
    }

    return row;
}

static void
on_install_done (GObject      *source,
                 GAsyncResult *res,
                 ExmWindow    *self)
{
    GError *error = NULL;
    if (!exm_manager_install_finish (EXM_MANAGER (source), res, &error) && error)
    {
        // TODO: Properly log this
        g_critical ("%s\n", error->message);
    }
}

static void
install_remote (GtkButton   *button,
                const gchar *uuid)
{
    GtkRoot *root = gtk_widget_get_root (GTK_WIDGET (button));
    ExmWindow *self = EXM_WINDOW (root);

    exm_manager_install_async (self->manager, uuid, NULL,
                               (GAsyncReadyCallback) on_install_done,
                               self);
}

static void
on_image_loaded (GObject      *source,
                 GAsyncResult *res,
                 GtkImage     *target)
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

    gtk_image_set_from_paintable (target, GDK_PAINTABLE (texture));
}

static GtkWidget *
create_thumbnail (ExmImageResolver *resolver,
                  const gchar      *icon_uri)
{
    GtkWidget *icon;

    icon = gtk_image_new ();
    gtk_widget_set_valign (icon, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (icon, GTK_ALIGN_CENTER);

    // Set to default icon
    gtk_image_set_from_resource (GTK_IMAGE (icon), "/com/mattjakeman/ExtensionManager/icons/plugin.png");

    // If not the default icon, lookup and lazily replace
    // TODO: There are some outstanding threading issues so avoid downloading for now
    /*if (strcmp (icon_uri, "/static/images/plugin.png") != 0)
    {
        exm_image_resolver_resolve_async (resolver, icon_uri, NULL,
                                          (GAsyncReadyCallback) on_image_loaded,
                                          icon);
    }*/

    return icon;
}

static GtkWidget *
search_widget_factory (ExmSearchResult *result,
                       ExmWindow       *self)
{
    GtkWidget *row;
    GtkWidget *box;
    GtkWidget *description_label;
    GtkWidget *button_box;
    GtkWidget *install_btn;
    GtkWidget *link_btn;
    GtkWidget *icon;
    GtkWidget *screenshot;

    gchar *uri;

    gchar *uuid, *name, *creator, *icon_uri, *screenshot_uri, *link, *description;
    g_object_get (result,
                  "uuid", &uuid,
                  "name", &name,
                  "creator", &creator,
                  "icon", &icon_uri,
                  "screenshot", &screenshot_uri,
                  "link", &link,
                  "description", &description,
                  NULL);

    name = g_markup_escape_text (name, -1);
    uri = g_strdup_printf ("https://extensions.gnome.org/%s", link);

    row = adw_expander_row_new ();

    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), name);
    adw_expander_row_set_subtitle (ADW_EXPANDER_ROW (row), creator);

    icon = create_thumbnail (self->resolver, icon_uri);
    adw_expander_row_add_prefix (ADW_EXPANDER_ROW (row), icon);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class (box, "content");
    adw_expander_row_add_row (ADW_EXPANDER_ROW (row), box);

    description_label = gtk_label_new (description);
    gtk_label_set_xalign (GTK_LABEL (description_label), 0);
    gtk_label_set_wrap (GTK_LABEL (description_label), GTK_WRAP_WORD);
    gtk_widget_add_css_class (description_label, "description");
    gtk_box_append (GTK_BOX (box), description_label);

    // TODO: This should be on-demand otherwise we're downloading far too often
    /*screenshot = gtk_image_new ();
    exm_image_resolver_resolve_async (self->resolver, screenshot_uri, NULL, (GAsyncReadyCallback)on_image_loaded, screenshot);
    gtk_box_append (GTK_BOX (box), screenshot);*/

    button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign (button_box, GTK_ALIGN_END);
    gtk_box_append (GTK_BOX (box), button_box);

    link_btn = gtk_link_button_new_with_label (uri, _("Go to Page"));
    gtk_box_append (GTK_BOX (button_box), link_btn);

    install_btn = gtk_button_new_with_label (_("Install"));
    g_signal_connect (install_btn, "clicked", G_CALLBACK (install_remote), uuid);
    gtk_box_append (GTK_BOX (button_box), install_btn);

    if (exm_manager_is_installed_uuid (self->manager, uuid))
    {
        gtk_button_set_label (GTK_BUTTON (install_btn), _("Installed"));
        gtk_widget_set_sensitive (install_btn, FALSE);
    }

    return row;
}

static void
refresh_search (ExmWindow *self)
{
    gtk_list_box_bind_model (self->search_results, self->search_results_model,
                             (GtkListBoxCreateWidgetFunc) search_widget_factory,
                             g_object_ref (self), g_object_unref);
}

static void
on_search_result (GObject      *source,
                  GAsyncResult *res,
                  ExmWindow    *self)
{
    GError *error = NULL;

    self->search_results_model = exm_search_provider_query_finish (EXM_SEARCH_PROVIDER (source), res, &error);

    refresh_search (self);
}

static void
search (ExmWindow *self, const gchar *query)
{
    // TODO: Loading Indicator
    exm_search_provider_query_async (self->search, query, NULL,
                                     (GAsyncReadyCallback) on_search_result,
                                     self);
}

static void
on_search_changed (GtkSearchEntry *search_entry,
                   ExmWindow      *self)
{
    const char *query = gtk_editable_get_text (GTK_EDITABLE (search_entry));
    search (self, query);
}

static void
update_extensions_list (ExmWindow   *self,
                        GtkListBox  *list_box,
                        const gchar *property_name)
{
    GListModel *model;

    g_object_get (self->manager, property_name, &model, NULL);
    gtk_list_box_bind_model (list_box, model,
                             (GtkListBoxCreateWidgetFunc) widget_factory,
                             NULL, NULL);

    refresh_search (self);
}

static void
update_user_extensions_list (ExmWindow *self)
{
    update_extensions_list (self, self->user_list_box, "user-extensions");
}

static void
update_system_extensions_list (ExmWindow *self)
{
    update_extensions_list (self, self->system_list_box, "system-extensions");
}

static void
exm_window_init (ExmWindow *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    self->manager = exm_manager_new ();
    self->resolver = exm_image_resolver_new ();
    self->search = exm_search_provider_new ();

    g_signal_connect_swapped (self->manager,
                              "notify::user-extensions",
                              G_CALLBACK (update_user_extensions_list),
                              self);

    g_signal_connect_swapped (self->manager,
                              "notify::system-extensions",
                              G_CALLBACK (update_system_extensions_list),
                              self);

    update_user_extensions_list (self);
    update_system_extensions_list (self);

    g_signal_connect (self->search_entry,
                      "search-changed",
                      G_CALLBACK (on_search_changed),
                      self);

    // Fire off a default search
    search (self, "");
}
