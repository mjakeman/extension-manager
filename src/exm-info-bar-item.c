#include "exm-info-bar-item.h"

struct _ExmInfoBarItem
{
    AdwBin parent_instance;

    GtkImage *icon;
    GtkLabel *title;
    GtkLabel *subtitle;
};

G_DEFINE_FINAL_TYPE (ExmInfoBarItem, exm_info_bar_item, ADW_TYPE_BIN)

enum {
    PROP_0,
    PROP_ICON,
    PROP_TITLE,
    PROP_SUBTITLE,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmInfoBarItem *
exm_info_bar_info_item_new (void)
{
    return g_object_new (EXM_TYPE_INFO_BAR_ITEM, NULL);
}

static void
exm_info_bar_item_finalize (GObject *object)
{
    ExmInfoBarItem *self = (ExmInfoBarItem *)object;

    G_OBJECT_CLASS (exm_info_bar_item_parent_class)->finalize (object);
}

GtkLabel *
exm_info_bar_item_get_subtitle (ExmInfoBarItem *self)
{
    g_return_val_if_fail (EXM_IS_INFO_BAR_ITEM (self), NULL);

    return self->subtitle;
}

void
exm_info_bar_item_set_subtitle (ExmInfoBarItem *self,
                                const char     *subtitle)
{
    g_return_if_fail (EXM_IS_INFO_BAR_ITEM (self));

    if (g_strcmp0 (gtk_label_get_label (self->subtitle), subtitle) == 0)
      return;

    gtk_label_set_label (self->subtitle, subtitle);

    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SUBTITLE]);
}

static void
exm_info_bar_item_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
    ExmInfoBarItem *self = EXM_INFO_BAR_ITEM (object);

    switch (prop_id)
    {
    case PROP_ICON:
        g_value_set_string (value, gtk_image_get_icon_name (self->icon));
        break;
    case PROP_TITLE:
        g_value_set_string (value, gtk_label_get_label (self->title));
        break;
    case PROP_SUBTITLE:
        g_value_set_string (value, gtk_label_get_text (exm_info_bar_item_get_subtitle (self)));
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_info_bar_item_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
    ExmInfoBarItem *self = EXM_INFO_BAR_ITEM (object);

    switch (prop_id)
    {
    case PROP_ICON:
        gtk_image_set_from_icon_name (self->icon, g_value_get_string (value));
        break;
    case PROP_TITLE:
        gtk_label_set_label (self->title, g_value_get_string (value));
        break;
    case PROP_SUBTITLE:
        exm_info_bar_item_set_subtitle (self, g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_info_bar_item_class_init (ExmInfoBarItemClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_info_bar_item_finalize;
    object_class->get_property = exm_info_bar_item_get_property;
    object_class->set_property = exm_info_bar_item_set_property;

    properties [PROP_ICON] =
        g_param_spec_string ("icon",
                             "Icon",
                             "The icon for the item",
                             NULL,
                             (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    properties [PROP_TITLE] =
        g_param_spec_string ("title",
                             "Title",
                             "The title for the item",
                             NULL,
                             (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    properties [PROP_SUBTITLE] =
        g_param_spec_string ("subtitle",
                             "Subtitle",
                             "The subtitle for the item",
                             NULL,
                             (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-info-bar-item.ui");

    g_type_ensure (EXM_TYPE_INFO_BAR_ITEM);

    gtk_widget_class_bind_template_child (widget_class, ExmInfoBarItem, icon);
    gtk_widget_class_bind_template_child (widget_class, ExmInfoBarItem, title);
    gtk_widget_class_bind_template_child (widget_class, ExmInfoBarItem, subtitle);
}

static void
exm_info_bar_item_init (ExmInfoBarItem *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}
