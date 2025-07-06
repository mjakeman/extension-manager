/*
 * exm-screenshot-view.c
 *
 * Copyright 2022-2025 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#include "exm-screenshot-view.h"

#include "exm-zoom-picture.h"

#include "exm-config.h"

struct _ExmScreenshotView
{
    AdwNavigationPage parent_instance;

    ExmZoomPicture *overlay_screenshot;

    GdkPaintable *screenshot;
};

G_DEFINE_FINAL_TYPE (ExmScreenshotView, exm_screenshot_view, ADW_TYPE_NAVIGATION_PAGE)

enum {
    PROP_0,
    PROP_SCREENSHOT,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

void
set_screenshot (ExmScreenshotView *self,
                GdkPaintable *paintable);

ExmScreenshotView *
exm_screenshot_view_new (void)
{
    return g_object_new (EXM_TYPE_SCREENSHOT_VIEW, NULL);
}

static void
exm_screenshot_view_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
    ExmScreenshotView *self = EXM_SCREENSHOT_VIEW (object);

    switch (prop_id)
    {
    case PROP_SCREENSHOT:
        g_value_set_object (value, self->screenshot);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_screenshot_view_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
    ExmScreenshotView *self = EXM_SCREENSHOT_VIEW (object);

    switch (prop_id)
    {
    case PROP_SCREENSHOT:
        self->screenshot = g_value_get_object (value);
        set_screenshot (self, self->screenshot);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

void
set_screenshot (ExmScreenshotView *self,
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
             GParamSpec        *pspec G_GNUC_UNUSED,
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
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = exm_screenshot_view_get_property;
    object_class->set_property = exm_screenshot_view_set_property;

    properties [PROP_SCREENSHOT]
        = g_param_spec_object ("screenshot",
                               "Screenshot",
                               "Screenshot",
                               GDK_TYPE_PAINTABLE,
                               G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

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
