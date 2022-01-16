#include "exm-browse-page.h"

#include "local/exm-manager.h"

#include "web/exm-search-provider.h"
#include "web/exm-image-resolver.h"

#include "web/model/exm-search-result.h"

#include <glib/gi18n.h>

struct _ExmBrowsePage
{
    GtkWidget parent_instance;

    ExmSearchProvider *search;
    ExmImageResolver *resolver;
    ExmManager *manager;

    GListModel *search_results_model;

    // Template Widgets
    GtkSearchEntry      *search_entry;
    GtkListBox          *search_results;
    GtkStack            *search_stack;
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

static void
install_remote (GtkButton   *button,
                const gchar *uuid)
{
    gtk_widget_activate_action (GTK_WIDGET (button),
                                "ext.install",
                                "s", uuid);
}

static GtkWidget *
search_widget_factory (ExmSearchResult *result,
                       ExmBrowsePage   *self)
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
    gtk_label_set_wrap (GTK_LABEL (description_label), GTK_WRAP_WORD_CHAR);
    // This seems to fix a strange wrapping error?
    gtk_label_set_wrap_mode (GTK_LABEL (description_label), PANGO_WRAP_WORD_CHAR);
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
refresh_search (ExmBrowsePage *self)
{
    if (!self->manager)
        return;

    gtk_list_box_bind_model (self->search_results, self->search_results_model,
                             (GtkListBoxCreateWidgetFunc) search_widget_factory,
                             g_object_ref (self), g_object_unref);

    // Show Loading Indicator
    gtk_stack_set_visible_child_name (self->search_stack, "page_results");
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
        const gchar   *query)
{
    // Show Loading Indicator
    gtk_stack_set_visible_child_name (self->search_stack, "page_spinner");

    exm_search_provider_query_async (self->search, query, NULL,
                                     (GAsyncReadyCallback) on_search_result,
                                     self);
}

static void
on_search_changed (GtkSearchEntry *search_entry,
                   ExmBrowsePage  *self)
{
    const char *query = gtk_editable_get_text (GTK_EDITABLE (search_entry));
    search (self, query);
}

static void
on_search_entry_realize (GtkSearchEntry *search_entry,
                         ExmBrowsePage  *self)
{
    // Fire off a default search
    search (self, "");
}

static void
on_bind_manager (ExmBrowsePage *self)
{
    GListModel *user_ext_model;
    GListModel *system_ext_model;

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

    refresh_search (self);
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

    gtk_widget_class_bind_template_callback (widget_class, on_search_entry_realize);

    gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
exm_browse_page_init (ExmBrowsePage *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    self->search = exm_search_provider_new ();

    g_signal_connect (self->search_entry,
                      "search-changed",
                      G_CALLBACK (on_search_changed),
                      self);

    g_signal_connect (self->search_entry,
                      "realize",
                      G_CALLBACK (on_search_entry_realize),
                      self);

    g_signal_connect (self,
                      "notify::manager",
                      G_CALLBACK (on_bind_manager),
                      NULL);
}
