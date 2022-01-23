#include "exm-detail-view.h"

#include "web/exm-data-provider.h"
#include "web/exm-image-resolver.h"
#include "local/exm-manager.h"

#include <glib/gi18n.h>

struct _ExmDetailView
{
    GtkBox parent_instance;

    ExmManager *manager;
    ExmDataProvider *provider;
    ExmImageResolver *resolver;
    GCancellable *resolver_cancel;

    gchar *uuid;
    int pk;

    AdwWindowTitle *title;
    GtkStack *stack;
    GtkButton *ext_install;
    GtkLabel *ext_description;
    GtkLabel *ext_title;
    GtkLabel *ext_author;
    GtkStack *ext_screenshot_stack;
    GtkPicture *ext_screenshot;
};

G_DEFINE_FINAL_TYPE (ExmDetailView, exm_detail_view, GTK_TYPE_BOX)

enum {
    PROP_0,
    PROP_MANAGER,
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
                       InstallButtonState *state)
{
    gtk_widget_remove_css_class (GTK_WIDGET (button), "warning");
    gtk_widget_remove_css_class (GTK_WIDGET (button), "suggested-action");

    switch ((int)state)
    {
    case STATE_DEFAULT:
        gtk_button_set_label (button, _("Install"));
        gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
        gtk_widget_add_css_class (GTK_WIDGET (button), "suggested-action");
        break;
    case STATE_INSTALLED:
        gtk_button_set_label (button, _("Installed"));
        gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
        break;
    case STATE_UNSUPPORTED:
        gtk_button_set_label (button, _("Unsupported"));
        gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
        gtk_widget_add_css_class (GTK_WIDGET (button), "warning");
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

    gtk_picture_set_paintable (self->ext_screenshot, GDK_PAINTABLE (texture));
    gtk_stack_set_visible_child_name (self->ext_screenshot_stack, "page_picture");
    g_object_unref (texture);
    g_object_unref (self);
}

static void
queue_resolve_screenshot (ExmDetailView    *self,
                          ExmImageResolver *resolver,
                          const gchar      *screenshot_uri,
                          GCancellable     *cancellable)
{
    exm_image_resolver_resolve_async (resolver, screenshot_uri, cancellable,
                                      (GAsyncReadyCallback) on_image_loaded,
                                      g_object_ref (self));
}

static void
on_data_loaded (GObject      *source,
                GAsyncResult *result,
                gpointer      user_data)
{
    ExmSearchResult *data;
    GError *error = NULL;
    ExmDetailView *self;

    self = EXM_DETAIL_VIEW (user_data);

    if ((data = exm_data_provider_get_finish (EXM_DATA_PROVIDER (source), result, &error)) != FALSE)
    {
        gboolean is_installed;
        gchar *uuid, *name, *creator, *icon_uri, *screenshot_uri, *link, *description;
        g_object_get (data,
                      "uuid", &uuid,
                      "name", &name,
                      "creator", &creator,
                      "icon", &icon_uri,
                      "screenshot", &screenshot_uri,
                      "link", &link,
                      "description", &description,
                      NULL);

        adw_window_title_set_title (self->title, name);
        adw_window_title_set_subtitle (self->title, uuid);

        is_installed = exm_manager_is_installed_uuid (self->manager, uuid);

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

            gtk_picture_set_paintable (self->ext_screenshot, NULL);
            gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot_stack), TRUE);
            gtk_stack_set_visible_child_name (self->ext_screenshot_stack, "page_spinner");

            queue_resolve_screenshot (self, self->resolver, screenshot_uri, self->resolver_cancel);
        }
        else
        {
            gtk_widget_set_visible (GTK_WIDGET (self->ext_screenshot_stack), FALSE);
        }

        gtk_actionable_set_action_target (GTK_ACTIONABLE (self->ext_install), "s", uuid);
        gtk_actionable_set_action_name (GTK_ACTIONABLE (self->ext_install), "ext.install");
        install_btn_set_state (self->ext_install, is_installed ? STATE_INSTALLED : STATE_DEFAULT);

        gtk_stack_set_visible_child_name (self->stack, "page_detail");

        return;
    }

    adw_window_title_set_title (self->title, _("An Error Occurred"));
    adw_window_title_set_subtitle (self->title, NULL);
}

void
exm_detail_view_load_for_uuid (ExmDetailView *self,
                               const gchar   *uuid,
                               int            pk)
{
    // g_assert (gtk_widget_is_constructed)

    self->uuid = uuid;
    self->pk = pk;

    /* Translators: Use unicode ellipsis '…' rather than three dots '...' */
    adw_window_title_set_title (self->title, _("Loading…"));
    adw_window_title_set_subtitle (self->title, NULL);

    gtk_stack_set_visible_child_name (self->stack, "page_spinner");

    exm_data_provider_get_async (self->provider, uuid, pk, NULL, on_data_loaded, self);
}

void
exm_detail_view_update (ExmDetailView *self)
{
    // Check if the newly installed extension is the
    // one being displayed in this detail view
    if (exm_manager_is_installed_uuid (self->manager, self->uuid))
    {
        install_btn_set_state (self->ext_install, STATE_INSTALLED);
    }
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

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-detail-view.ui");

    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, title);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, stack);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_title);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_author);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_description);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_install);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_screenshot_stack);
    gtk_widget_class_bind_template_child (widget_class, ExmDetailView, ext_screenshot);
}

static void
exm_detail_view_init (ExmDetailView *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    self->provider = exm_data_provider_new ();
    self->resolver = exm_image_resolver_new ();
}
