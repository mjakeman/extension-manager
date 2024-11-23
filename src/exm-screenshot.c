/* exm-screenshot.c
 *
 * Copyright 2022-2024 Matthew Jakeman <mjakeman26@outlook.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "exm-screenshot.h"

#include "exm-zoom-picture.h"

#include "exm-config.h"

struct _ExmScreenshot
{
    GtkWidget parent_instance;

    GtkPicture *picture;
    GtkStack *stack;
};

G_DEFINE_FINAL_TYPE (ExmScreenshot, exm_screenshot, GTK_TYPE_WIDGET)

ExmScreenshot *
exm_screenshot_new (void)
{
    return g_object_new (EXM_TYPE_SCREENSHOT, NULL);
}

static void
exm_screenshot_finalize (GObject *object)
{
    GtkWidget *child;
    ExmScreenshot *self = (ExmScreenshot *)object;

    child = gtk_widget_get_first_child (GTK_WIDGET (self));
    gtk_widget_unparent (child);

    G_OBJECT_CLASS (exm_screenshot_parent_class)->finalize (object);
}

void
exm_screenshot_set_paintable (ExmScreenshot *self,
                              GdkPaintable  *paintable)
{
    gtk_picture_set_paintable (self->picture, paintable);
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

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-screenshot.ui", RESOURCE_PATH));

    widget_class->get_request_mode = exm_screenshot_get_request_mode;
    widget_class->measure = exm_screenshot_measure;
    widget_class->size_allocate = exm_screenshot_size_allocate;

	g_type_ensure (EXM_TYPE_ZOOM_PICTURE);

    gtk_widget_class_bind_template_child (widget_class, ExmScreenshot, picture);
    gtk_widget_class_bind_template_child (widget_class, ExmScreenshot, stack);

    gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_IMG);

}

static void
exm_screenshot_init (ExmScreenshot *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}
