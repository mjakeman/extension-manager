#include "exm-detail-view.h"

#include "exm-screenshot.h"
#include "exm-comment-tile.h"

#include "web/exm-data-provider.h"
#include "web/exm-image-resolver.h"
#include "web/exm-comment-provider.h"
#include "web/model/exm-shell-version-map.h"
#include "web/model/exm-comment.h"
#include "local/exm-manager.h"

#include <glib/gi18n.h>

struct _ExmDetailView
{
    GtkBox parent_instance;

    ExmManager *manager;
    ExmDataProvider *provider;
    ExmImageResolver *resolver;
    ExmCommentProvider *comment_provider;
    GCancellable *resolver_cancel;

    gchar *shell_version;
    gchar *uuid;

    AdwWindowTitle *title;
    GtkStack *stack;
    GtkButton *ext_install;
    GtkLabel *ext_description;
    GtkLabel *ext_title;
    GtkLabel *ext_author;
    ExmScreenshot *ext_screenshot;
    GtkFlowBox *supported_versions;
    GtkScrolledWindow *scroll_area;
    GtkFlowBox *comment_box;
    GtkButton *show_more_btn;

    AdwActionRow *link_extensions;
    gchar *uri_extensions;
};

G_DEFINE_FINAL_TYPE (ExmDetailView, exm_detail_view, GTK_TYPE_BOX)

enum {
    PROP_0,
    PROP_MANAGER,
    PROP_SHELL_VERSION,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmDetailView *
exm_detail_view_new (void)
{
    return g_object_new (EXM_TYPE_DETAIL_VIEW, NULL);
}

static void
exm_detail_view_finalize (GObject *object)
{
    ExmDetailView *self = (ExmDetailView *)object;

    G_OBJECT_CLASS (exm_detail_view_parent_class)->finalize (object);
}

static void
exm_detail_view_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
    ExmDetailView *self = EXM_DETAIL_VIEW (object);

    switch (prop_id)
    {
    case PROP_MANAGER:
        g_value_set_object (value, self->manager);
        break;
    case PROP_SHELL_VERSION:
        g_value_set_string (value, self->shell_version);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void on_bind_manager (ExmDetailView *self);

static void
exm_detail_view_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
    ExmDetailView *self = EXM_DETAIL_VIEW (object);

    switch (prop_id)
    {
    case PROP_MANAGER:
        self->manager = g_value_get_object (value);
        on_bind_manager (self);
        break;
    case PROP_SHELL_VERSION:
        self->shell_version = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

typedef enum
{
    STATE_DEFAULT,
    STATE_INSTALLED,
    STATE_UNSUPPORTED
} InstallButtonState;

void
install_btn_set_state (GtkButton          *button,
                       InstallButtonState  state)
{
    const gchar *tooltip;

    tooltip = _("This extension is incompatible with your current version of GNOME.");

    gtk_widget_remove_css_class (GTK_WIDGET (button), "warning");
    gtk_widget_remove_css_class (GTK_WIDGET (button), "suggested-action");
    gtk_widget_set_tooltip_text (GTK_WIDGET (button), NULL);

    switch ((int)state)
    {
    case STATE_DEFAULT:
        gtk_button_set_label (button, _("Install"));
        gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
        gtk_widget_add_css_class (GTK_WIDGET (button), "suggested-action");
        break;
    case STATE_INSTALLED:
        gtk_button_set_label (button, C_("State", "Installed"));
        gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
        break;
    case STATE_UNSUPPORTED:
        gtk_button_set_label (button, _("Unsupported"));
        gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
        gtk_widget_add_css_class (GTK_WIDGET (button), "warning");
        gtk_widget_set_tooltip_text (GTK_WIDGET (button), tooltip);
        break;
    }
}

static void
on_image_loaded (GObject       *source,
                 GAsyncResult  *res,
                 ExmDetailView *self)
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

    exm_screenshot_set_paintable (self->ext_screenshot, GDK_PAINTABLE (texture));
    exm_screenshot_display (self->ext_screenshot);
    g_object_unref (texture);
    g_object_unref (self);
}

static void
queue_resolve_screenshot (ExmDetailView    *self,
                          const gchar      *screenshot_uri,
                          GCancellable     *cancellable)
{
    exm_image_resolver_resolve_async (self->resolver, screenshot_uri, cancellable,
                                      (GAsyncReadyCallback) on_image_loaded,
                                      g_object_ref (self));
}

static GtkWidget *
comment_factory (ExmComment    *comment,
                 ExmDetailView *self)
{
    return GTK_WIDGET (exm_comment_tile_new (comment));
}

static void
on_get_comments (GObject       *source,
                 GAsyncResult  *res,
                 ExmDetailView *self)
{
    GError *error = NULL;

    GListModel *model = exm_comment_provider_get_comments_finish (EXM_COMMENT_PROVIDER (source), res, &error);

    gtk_flow_box_bind_model (self->comment_box, model,
                             (GtkListBoxCreateWidgetFunc) comment_factory,
                             g_object_ref (self), g_object_unref);
}

static void
queue_resolve_comments (ExmDetailView *self,
                        gint           pk,
                        GCancellable  *cancellable)
{
    exm_comment_provider_get_comments_async (self->comment_provider, pk, cancellable,
                                             (GAsyncReadyCallback) on_get_comments,
                                             self);
}

static void
on_data_loaded (GObject      *source,
                GAsyncResult *result,
                gpointer      user_data)
{
    ExmSearchResult *data;
    GError *error = NULL;
    ExmDetailView *self;
    InstallButtonState install_state;
    GtkWidget *child;
    GList *version_iter;
    ExmShellVersionMap *version_map;
    gchar *uri;

    self = EXM_DETAIL_VIEW (user_data);

    if ((data = exm_data_provider_get_finish (EXM_DATA_PROVIDER (source), result, &error)) != FALSE)
    {
        gint pk;
        gboolean is_installed, is_supported;
        gchar *uuid, *name, *creator, *icon_uri, *screenshot_uri, *link, *description;
        g_object_get (data,
                      "uuid", &uuid,
                      "name", &name,
                      "creator", &creator,
                      "icon", &icon_uri,
                      "screenshot", &screenshot_uri,
                      "link", &link,
                      "description", &description,
                      "shell_version_map", &version_map,
                      "pk", &pk,
                      NULL);

        adw_window_title_set_title (self->title, name);
        adw_window_title_set_subtitle (self->title, uuid);

        is_installed = exm_manager_is_installed_uuid (self->manager, uuid);
        is_supported = exm_search_result_supports_shell_version (data, self->shell_version);

        gtk_label_set_label (self->ext_title, name);
        gtk_label_set_label (self->ext_author, creator);
        gtk_label_set_label (self->ext_description, description);

        if (self->resolver_cancel)
        {
            g_cancellable_cancel (self->resolver_cancel);
            g_clear_object (&self->resolver_cancel);
        }

        if (screenshot_uri != NULL)
        {
            self->resolver_cancel = g_cancellable_new ();

            exm_screenshot_set_paintable (self->ext_screenshot, NULL);
            gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot), TRUE);
            exm_screenshot_reset (self->ext_screenshot);

            queue_resolve_screenshot (self, screenshot_uri, self->resolver_cancel);
        }
        else
        {
            gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot), FALSE);
        }

        install_state = is_installed
            ? STATE_INSTALLED
            : (is_supported
               ? STATE_DEFAULT
               : STATE_UNSUPPORTED);

        gtk_actionable_set_action_target (GTK_ACTIONABLE (self->ext_install), "s", uuid);
        gtk_actionable_set_action_name (GTK_ACTIONABLE (self->ext_install), "ext.install");
        install_btn_set_state (self->ext_install, install_state);

        self->uri_extensions = g_strdup_printf ("https://extensions.gnome.org/%s", link);
        adw_action_row_set_subtitle (self->link_extensions, self->uri_extensions);

        // Clear Flowbox
        while ((child = gtk_widget_get_first_child (GTK_WIDGET (self->supported_versions))))
            gtk_flow_box_remove (self->supported_versions, child);

        for (version_iter = version_map->map;
             version_iter != NULL;
             version_iter = version_iter->next)
        {
            gchar *version;
            MapEntry *entry;
            GtkWidget *label;

            entry = version_iter->data;

            if (entry->shell_minor_version)
                version = g_strdup_printf ("%s.%s", entry->shell_major_version, entry->shell_minor_version);
            else
                version = g_strdup (entry->shell_major_version);

            label = gtk_label_new (version);
            gtk_widget_add_css_class (label, "version-label");
            gtk_flow_box_prepend (self->supported_versions, label);

            g_free (version);
        }

        queue_resolve_comments (self, pk, self->resolver_cancel);

        // Reset scroll position
        gtk_adjustment_set_value (gtk_scrolled_window_get_vadjustment (self->scroll_area), 0);

        gtk_stack_set_visible_child_name (self->stack, "page_detail");

        return;
    }

    adw_window_title_set_title (self->title, _("An Error Occurred"));
    adw_window_title_set_subtitle (self->title, NULL);
    gtk_stack_set_visible_child_name (self->stack, "page_error");
}

void
exm_detail_view_load_for_uuid (ExmDetailView *self,
                               const gchar   *uuid)
{
    // g_assert (gtk_widget_is_constructed)

    self->uuid = uuid;

    /* Translators: Use unicode ellipsis '…' rather than three dots '...' */
    adw_window_title_set_title (self->title, _("Loading…"));
    adw_window_title_set_subtitle (self->title, NULL);

    gtk_stack_set_visible_child_name (self->stack, "page_spinner");

    exm_data_provider_get_async (self->provider, uuid, NULL, on_data_loaded, self);
}

void
exm_detail_view_update (ExmDetailView *self)
{
    if (!self->uuid)
        return;

    // Check if the newly installed extension is the
    // one being displayed in this detail view
    if (exm_manager_is_installed_uuid (self->manager, self->uuid))
    {
        install_btn_set_state (self->ext_install, STATE_INSTALLED);
    }
}

static void
open_link (ExmDetailView *self,
           const char    *action_name,
           GVariant      *param)
{
    GtkWidget *toplevel;
    gchar *uri = NULL;

    toplevel = GTK_WIDGET (gtk_widget_get_root (GTK_WIDGET (self)));

    if (strcmp (action_name, "detail.open-extensions") == 0)
        uri = self->uri_extensions;
    else if (strcmp (action_name, "detail.open-homepage") == 0)
        g_warning ("open_link(): cannot open homepage as not yet implemented.");
    else
        g_critical ("open_link() invalid action: %s", action_name);

    gtk_show_uri (GTK_WINDOW (toplevel), uri, GDK_CURRENT_TIME);
}

static void
on_bind_manager (ExmDetailView *self)
{
    GListModel *user_ext_model;
    GListModel *system_ext_model;

    g_object_get (self->manager,
                  "user-extensions", &user_ext_model,
                  "system-extensions", &system_ext_model,
                  NULL);

    g_signal_connect_swapped (user_ext_model,
                              "items-changed",
                              G_CALLBACK (exm_detail_view_update),
                              self);

    g_signal_connect_swapped (system_ext_model,
                              "items-changed",
                              G_CALLBACK (exm_detail_view_update),
                              self);
}

static void
exm_detail_view_class_init (ExmDetailViewClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_detail_view_finalize;
    object_class->get_property = exm_detail_view_get_property;
    object_class->set_property = exm_detail_view_set_property;

    properties [PROP_MANAGER]
        = g_param_spec_object ("manager",
                               "Manager",
                               "Manager",
                               EXM_TYPE_MANAGER,
                               G_PARAM_READWRITE);

    properties [PROP_SHELL_VERSION]
        = g_param_spec_string ("shell-version",
                               "Shell Version",
                               "Shell Version",
                               NULL,
                               G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-detail-view.ui");

    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, title);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, stack);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_title);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_author);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_description);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_install);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_screenshot);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, supported_versions);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, link_extensions);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, scroll_area);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, comment_box);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, show_more_btn);

    gtk_widget_class_install_action (widget_class, "detail.open-extensions", NULL, open_link);
    gtk_widget_class_install_action (widget_class, "detail.open-homepage", NULL, open_link);
}

static void
exm_detail_view_init (ExmDetailView *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    self->provider = exm_data_provider_new ();
    self->resolver = exm_image_resolver_new ();
    self->comment_provider = exm_comment_provider_new ();
}
