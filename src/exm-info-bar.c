#include "exm-info-bar.h"

#include "exm-info-bar-item.h"

#include <glib/gi18n.h>

struct _ExmInfoBar
{
    GtkBox parent_instance;

    ExmInfoBarItem *downloads_item;
    ExmInfoBarItem *version_item;

    guint signal_handler;
};

G_DEFINE_FINAL_TYPE (ExmInfoBar, exm_info_bar, GTK_TYPE_BOX)

enum {
    PROP_0,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmInfoBar *
exm_info_bar_new (void)
{
    return g_object_new (EXM_TYPE_INFO_BAR, NULL);
}

static void
exm_info_bar_finalize (GObject *object)
{
    ExmInfoBar *self = (ExmInfoBar *)object;

    G_OBJECT_CLASS (exm_info_bar_parent_class)->finalize (object);
}

static void
exm_info_bar_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    ExmInfoBar *self = EXM_INFO_BAR (object);

    switch (prop_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_info_bar_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    ExmInfoBar *self = EXM_INFO_BAR (object);

    switch (prop_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

void
exm_info_bar_set_downloads (ExmInfoBar   *self,
                            guint         downloads)
{
    exm_info_bar_item_set_subtitle (self->downloads_item, g_strdup_printf ("%'d", downloads));
}

void
exm_info_bar_set_version (ExmInfoBar   *self,
                          double        version)
{
    exm_info_bar_item_set_subtitle (self->version_item, version == -1 ? _("Unsupported") : g_strdup_printf("%.f", version));
}

static void
exm_info_bar_class_init (ExmInfoBarClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_info_bar_finalize;
    object_class->get_property = exm_info_bar_get_property;
    object_class->set_property = exm_info_bar_set_property;

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-info-bar.ui");

    g_type_ensure (EXM_TYPE_INFO_BAR);

    gtk_widget_class_bind_template_child (widget_class, ExmInfoBar, downloads_item);
    gtk_widget_class_bind_template_child (widget_class, ExmInfoBar, version_item);
}

static void
exm_info_bar_init (ExmInfoBar *self)
{
    g_type_ensure (EXM_TYPE_INFO_BAR_ITEM);

    gtk_widget_init_template (GTK_WIDGET (self));
}
