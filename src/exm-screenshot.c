#include "exm-screenshot.h"

#include "exm-zoom-picture.h"

struct _ExmScreenshot
{
    GtkWidget parent_instance;

    ExmZoomPicture *picture;
    GtkStack *stack;
};

G_DEFINE_FINAL_TYPE (ExmScreenshot, exm_screenshot, GTK_TYPE_WIDGET)

enum {
    PROP_0,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmScreenshot *
exm_screenshot_new (void)
{
    return g_object_new (EXM_TYPE_SCREENSHOT, NULL);
}

static void
exm_screenshot_finalize (GObject *object)
{
    ExmScreenshot *self = (ExmScreenshot *)object;

    G_OBJECT_CLASS (exm_screenshot_parent_class)->finalize (object);
}

static void
exm_screenshot_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
    ExmScreenshot *self = EXM_SCREENSHOT (object);

    switch (prop_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_screenshot_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
    ExmScreenshot *self = EXM_SCREENSHOT (object);

    switch (prop_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

void
exm_screenshot_set_paintable (ExmScreenshot *self,
                              GdkPaintable  *paintable)
{
    exm_zoom_picture_set_paintable (self->picture, paintable);
}

void
exm_screenshot_reset (ExmScreenshot *self)
{
    gtk_stack_set_visible_child_name (self->stack, "page_spinner");
}

void
exm_screenshot_display (ExmScreenshot *self)
{
    gtk_stack_set_visible_child_name (self->stack, "page_picture");
}

static void
exm_screenshot_measure (GtkWidget      *widget,
                        GtkOrientation  orientation,
                        int             for_size,
                        int            *minimum,
                        int            *natural,
                        int            *minimum_baseline,
                        int            *natural_baseline)
{
    ExmScreenshot *self = EXM_SCREENSHOT (widget);

    if (orientation == GTK_ORIENTATION_VERTICAL)
    {
        gtk_widget_measure (GTK_WIDGET (self->stack), GTK_ORIENTATION_VERTICAL, for_size,
                            minimum, natural, minimum_baseline, natural_baseline);

        int height = (for_size / 16.0f) * 9.0f; // 16:9 images

        *natural = *minimum = height;
    }
    else if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_widget_measure (GTK_WIDGET (self->stack), GTK_ORIENTATION_HORIZONTAL, for_size,
                            minimum, natural, minimum_baseline, natural_baseline);
    }
}

static GtkSizeRequestMode
exm_screenshot_get_request_mode (GtkWidget *widget)
{
    return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
exm_screenshot_size_allocate (GtkWidget *widget,
                              int        width,
                              int        height,
                              int        baseline)
{
    ExmScreenshot *self = EXM_SCREENSHOT (widget);

    gtk_widget_size_allocate (GTK_WIDGET (self->stack),
                              &(const GtkAllocation){0, 0, width, height},
                              baseline);
}

static void
exm_screenshot_class_init (ExmScreenshotClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_screenshot_finalize;
    object_class->get_property = exm_screenshot_get_property;
    object_class->set_property = exm_screenshot_set_property;

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-screenshot.ui");

    widget_class->get_request_mode = exm_screenshot_get_request_mode;
    widget_class->measure = exm_screenshot_measure;
    widget_class->size_allocate = exm_screenshot_size_allocate;

	g_type_ensure (EXM_TYPE_ZOOM_PICTURE);

    gtk_widget_class_bind_template_child (widget_class, ExmScreenshot, picture);
    gtk_widget_class_bind_template_child (widget_class, ExmScreenshot, stack);

}

static void
exm_screenshot_init (ExmScreenshot *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}
