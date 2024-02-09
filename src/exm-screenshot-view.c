#include "exm-screenshot-view.h"

#include "exm-zoom-picture.h"

#include "exm-config.h"

struct _ExmScreenshotView
{
    AdwNavigationPage parent_instance;

    ExmZoomPicture *overlay_screenshot;
};

G_DEFINE_FINAL_TYPE (ExmScreenshotView, exm_screenshot_view, ADW_TYPE_NAVIGATION_PAGE)

ExmScreenshotView *
exm_screenshot_view_new (void)
{
    return g_object_new (EXM_TYPE_SCREENSHOT_VIEW, NULL);
}

void
exm_screenshot_view_set_screenshot (ExmScreenshotView *self,
                                    GdkPaintable *paintable)
{
    exm_zoom_picture_set_paintable (self->overlay_screenshot, paintable);
    exm_zoom_picture_set_zoom_level (self->overlay_screenshot, 1.0f);
}

void
exm_screenshot_view_zoom (ExmScreenshotView *self,
                          const char        *action_name)
{
  float zoom_level;
  float zoom_step;

  zoom_level = exm_zoom_picture_get_zoom_level (self->overlay_screenshot);
  zoom_step = exm_zoom_picture_get_zoom_level_step (self->overlay_screenshot);

  if (g_strcmp0 (action_name, "screenshot.zoom-in") == 0)
    exm_zoom_picture_set_zoom_level (self->overlay_screenshot, zoom_level + zoom_step);
  else if (g_strcmp0 (action_name, "screenshot.zoom-out") == 0)
    exm_zoom_picture_set_zoom_level (self->overlay_screenshot, zoom_level - zoom_step);
  else if (g_strcmp0 (action_name, "screenshot.zoom-reset") == 0)
    exm_zoom_picture_set_zoom_level (self->overlay_screenshot, 1.0f);
  else
    g_assert_not_reached ();
}

static void
notify_zoom (ExmZoomPicture    *picture,
             GParamSpec        *pspec,
             ExmScreenshotView *self)
{
    GtkRoot *toplevel;
    float zoom_level;

    toplevel = gtk_widget_get_root (GTK_WIDGET (self));
    zoom_level = exm_zoom_picture_get_zoom_level (picture);

    // Set action states
    gtk_widget_action_set_enabled (GTK_WIDGET (toplevel), "screenshot.zoom-in",
                                   zoom_level < exm_zoom_picture_get_zoom_level_max (picture));
    gtk_widget_action_set_enabled (GTK_WIDGET (toplevel), "screenshot.zoom-out",
                                   zoom_level > exm_zoom_picture_get_zoom_level_min (picture));
}

static void
exm_screenshot_view_class_init (ExmScreenshotViewClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-screenshot-view.ui", RESOURCE_PATH));

    gtk_widget_class_bind_template_child (widget_class, ExmScreenshotView, overlay_screenshot);

    gtk_widget_class_bind_template_callback (widget_class, notify_zoom);
}

static void
exm_screenshot_view_init (ExmScreenshotView *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}
