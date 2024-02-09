#include "exm-info-bar.h"

#include "exm-config.h"

#include <adwaita.h>
#include <glib/gi18n.h>

struct _ExmInfoBar
{
    GtkBox parent_instance;

    AdwActionRow *downloads_item;
    AdwActionRow *version_item;
};

G_DEFINE_FINAL_TYPE (ExmInfoBar, exm_info_bar, GTK_TYPE_BOX)

ExmInfoBar *
exm_info_bar_new (void)
{
    return g_object_new (EXM_TYPE_INFO_BAR, NULL);
}

void
exm_info_bar_set_downloads (ExmInfoBar   *self,
                            guint         downloads)
{
    adw_action_row_set_subtitle (self->downloads_item, g_strdup_printf ("%'d", downloads));
}

void
exm_info_bar_set_version (ExmInfoBar   *self,
                          double        version)
{
    adw_action_row_set_subtitle (self->version_item, version == -1
                                 ? _("Unsupported")
                                 : g_strdup_printf ("%.f", version));
}

static void
exm_info_bar_class_init (ExmInfoBarClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-info-bar.ui", RESOURCE_PATH));

    g_type_ensure (EXM_TYPE_INFO_BAR);

    gtk_widget_class_bind_template_child (widget_class, ExmInfoBar, downloads_item);
    gtk_widget_class_bind_template_child (widget_class, ExmInfoBar, version_item);
}

static void
exm_info_bar_init (ExmInfoBar *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}
