#include "exm-search-row.h"

#include <glib/gi18n.h>

struct _ExmSearchRow
{
    GtkListBoxRow parent_instance;

    ExmSearchResult *search_result;
    gboolean is_installed;
    gboolean is_supported;
    gchar *uuid;

    GtkLabel *description_label;
    GtkButton *install_btn;
    GtkLabel *title;
    GtkLabel *subtitle;
};

G_DEFINE_FINAL_TYPE (ExmSearchRow, exm_search_row, GTK_TYPE_LIST_BOX_ROW)

enum {
    PROP_0,
    PROP_SEARCH_RESULT,
    PROP_IS_INSTALLED,
    PROP_IS_SUPPORTED,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmSearchRow *
exm_search_row_new (ExmSearchResult *search_result,
                    gboolean         is_installed,
                    gboolean         is_supported)
{
    return g_object_new (EXM_TYPE_SEARCH_ROW,
                         "search-result", search_result,
                         "is-installed", is_installed,
                         "is-supported", is_supported,
                         NULL);
}

static void
exm_search_row_finalize (GObject *object)
{
    ExmSearchRow *self = (ExmSearchRow *)object;

    G_OBJECT_CLASS (exm_search_row_parent_class)->finalize (object);
}

static void
exm_search_row_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
    ExmSearchRow *self = EXM_SEARCH_ROW (object);

    switch (prop_id)
    {
    case PROP_SEARCH_RESULT:
        g_value_set_object (value, self->search_result);
        break;
    case PROP_IS_INSTALLED:
        g_value_set_boolean (value, self->is_installed);
        break;
    case PROP_IS_SUPPORTED:
        g_value_set_boolean (value, self->is_supported);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_search_row_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
    ExmSearchRow *self = EXM_SEARCH_ROW (object);

    switch (prop_id)
    {
    case PROP_SEARCH_RESULT:
        self->search_result = g_value_get_object (value);
        if (self->search_result)
        {
            // TODO: Bind here, rather than in constructed()
            g_object_get (self->search_result,
                          "uuid", &self->uuid,
                          NULL);
        }
        break;
    case PROP_IS_INSTALLED:
        self->is_installed = g_value_get_boolean (value);
        break;
    case PROP_IS_SUPPORTED:
        self->is_supported = g_value_get_boolean (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

/*static void
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
    if (strcmp (icon_uri, "/static/images/plugin.png") != 0)
    {
        exm_image_resolver_resolve_async (resolver, icon_uri, NULL,
                                          (GAsyncReadyCallback) on_image_loaded,
                                          icon);
    }

    return icon;
}*/

static void
install_remote (GtkButton   *button,
                const gchar *uuid)
{
    gtk_widget_activate_action (GTK_WIDGET (button),
                                "ext.install",
                                "s", uuid);
}

static void
exm_search_row_constructed (GObject *object)
{
    // TODO: This big block of property assignments is currently copy/pasted
    // from ExmExtension. We can replace this with GtkExpression lookups
    // once blueprint-compiler supports expressions.
    // (See https://gitlab.gnome.org/jwestman/blueprint-compiler/-/issues/5)

    ExmSearchRow *self = EXM_SEARCH_ROW (object);

    gchar *uri;
    int pk;

    gchar *uuid, *name, *creator, *icon_uri, *screenshot_uri, *link, *description;
    g_object_get (self->search_result,
                  "uuid", &uuid,
                  "name", &name,
                  "creator", &creator,
                  "icon", &icon_uri,
                  "screenshot", &screenshot_uri,
                  "link", &link,
                  "description", &description,
                  "pk", &pk,
                  NULL);

    name = g_markup_escape_text (name, -1);
    uri = g_strdup_printf ("https://extensions.gnome.org/%s", link);

    gtk_actionable_set_action_name (GTK_ACTIONABLE (self), "win.show-detail");
    gtk_actionable_set_action_target (GTK_ACTIONABLE (self), "(sn)", uuid, pk);

    // icon = create_thumbnail (self->resolver, icon_uri);
    // adw_expander_row_add_prefix (ADW_EXPANDER_ROW (row), icon);

    // TODO: This should be on-demand otherwise we're downloading far too often
    // screenshot = gtk_image_new ();
    // exm_image_resolver_resolve_async (self->resolver, screenshot_uri, NULL, (GAsyncReadyCallback)on_image_loaded, screenshot);
    // gtk_box_append (GTK_BOX (box), screenshot);

    gtk_label_set_label (self->title, name);
    gtk_label_set_label (self->subtitle, creator);
    gtk_label_set_label (self->description_label, description);

    if (self->is_installed)
    {
        gtk_button_set_label (self->install_btn, _("Installed"));
        gtk_widget_set_sensitive (GTK_WIDGET (self->install_btn), FALSE);
    }

    if (!self->is_supported)
    {
        const gchar *tooltip;

        tooltip = _("This extension is incompatible with your current version of GNOME.");

        gtk_button_set_label (self->install_btn, _("Incompatible"));
        gtk_widget_set_sensitive (GTK_WIDGET (self->install_btn), FALSE);
        gtk_widget_set_tooltip_text (GTK_WIDGET (self->install_btn), tooltip);
    }

    g_signal_connect (self->install_btn, "clicked", G_CALLBACK (install_remote), uuid);
}

static void
exm_search_row_class_init (ExmSearchRowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_search_row_finalize;
    object_class->get_property = exm_search_row_get_property;
    object_class->set_property = exm_search_row_set_property;
    object_class->constructed = exm_search_row_constructed;

    properties [PROP_SEARCH_RESULT] =
        g_param_spec_object ("search-result",
                             "Search Result",
                             "Search Result",
                             EXM_TYPE_SEARCH_RESULT,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_IS_INSTALLED] =
        g_param_spec_boolean ("is-installed",
                              "Is Installed",
                              "Is Installed",
                              FALSE,
                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_IS_SUPPORTED] =
        g_param_spec_boolean ("is-supported",
                              "Is Supported",
                              "Is Supported",
                              FALSE,
                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-search-row.ui");

    gtk_widget_class_bind_template_child (widget_class, ExmSearchRow, description_label);
    gtk_widget_class_bind_template_child (widget_class, ExmSearchRow, install_btn);
    gtk_widget_class_bind_template_child (widget_class, ExmSearchRow, title);
    gtk_widget_class_bind_template_child (widget_class, ExmSearchRow, subtitle);
}

static void
exm_search_row_init (ExmSearchRow *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}
