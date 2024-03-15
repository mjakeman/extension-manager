#include "exm-screenshot-view.h"

#include "exm-zoom-picture.h"

struct _ExmScreenshotView
{
    AdwNavigationPage parent_instance;

    GSimpleAction *zoom_in;
    GSimpleAction *zoom_out;
    GSimpleAction *zoom_reset;

    ExmZoomPicture *overlay_screenshot;
};

G_DEFINE_FINAL_TYPE (ExmScreenshotView, exm_screenshot_view, ADW_TYPE_NAVIGATION_PAGE)

ExmScreenshotView *
exm_screenshot_view_new (void)
{
    return g_object_new (EXM_TYPE_SCREENSHOT_VIEW, NULL);
}

static void
exm_screenshot_view_finalize (GObject *object)
{
    ExmScreenshotView *self = (ExmScreenshotView *)object;

    G_OBJECT_CLASS (exm_screenshot_view_parent_class)->finalize (object);
}

void
exm_screenshot_view_set_screenshot (ExmScreenshotView *self,
                                    GdkPaintable *paintable)
{
    exm_zoom_picture_set_paintable (self->overlay_screenshot, paintable);
    exm_zoom_picture_set_zoom_level (self->overlay_screenshot, 1.0f);
}

static void
notify_zoom (ExmZoomPicture    *picture,
             GParamSpec        *pspec,
             ExmScreenshotView *self)
{
    float zoom_level;
    float max_zoom;
    float min_zoom;

    zoom_level = exm_zoom_picture_get_zoom_level (picture);
    max_zoom = exm_zoom_picture_get_zoom_level_max (picture);
    min_zoom = exm_zoom_picture_get_zoom_level_min (picture);

    // Set action states
    if (zoom_level < max_zoom)
        g_simple_action_set_enabled (self->zoom_in, TRUE);
    if (zoom_level == max_zoom)
        g_simple_action_set_enabled (self->zoom_in, FALSE);
    if (zoom_level > min_zoom)
        g_simple_action_set_enabled (self->zoom_out, TRUE);
    if (zoom_level == min_zoom)
        g_simple_action_set_enabled (self->zoom_out, FALSE);
}

static void
exm_screenshot_view_class_init (ExmScreenshotViewClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_screenshot_view_finalize;

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-screenshot-view.ui");

    gtk_widget_class_bind_template_child (widget_class, ExmScreenshotView, overlay_screenshot);

    gtk_widget_class_bind_template_callback (widget_class, notify_zoom);

    gtk_widget_class_add_binding_action (widget_class, GDK_KEY_plus, GDK_CONTROL_MASK, "screenshot.zoom-in", NULL);
    gtk_widget_class_add_binding_action (widget_class, GDK_KEY_minus, GDK_CONTROL_MASK, "screenshot.zoom-out", NULL);
    gtk_widget_class_add_binding_action (widget_class, GDK_KEY_0, GDK_CONTROL_MASK, "screenshot.zoom-reset", NULL);
}

static void
exm_screenshot_view_init (ExmScreenshotView *self)
{
    GSimpleActionGroup *group;

    gtk_widget_init_template (GTK_WIDGET (self));

    self->zoom_in = g_simple_action_new ("zoom-in", NULL);
    g_signal_connect_swapped (self->zoom_in, "activate", G_CALLBACK (exm_zoom_picture_zoom_in), self->overlay_screenshot);

    self->zoom_out = g_simple_action_new ("zoom-out", NULL);
    g_signal_connect_swapped (self->zoom_out, "activate", G_CALLBACK (exm_zoom_picture_zoom_out), self->overlay_screenshot);

    self->zoom_reset = g_simple_action_new ("zoom-reset", NULL);
    g_signal_connect_swapped (self->zoom_reset, "activate", G_CALLBACK (exm_zoom_picture_zoom_reset), self->overlay_screenshot);

    group = g_simple_action_group_new ();
    g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (self->zoom_in));
    g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (self->zoom_out));
    g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (self->zoom_reset));
    gtk_widget_insert_action_group (GTK_WIDGET (self), "screenshot", G_ACTION_GROUP (group));
}
