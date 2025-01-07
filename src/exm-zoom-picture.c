/*
 * exm-zoom-picture.c
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

#include "exm-zoom-picture.h"

struct _ExmZoomPicture
{
	GtkWidget parent_instance;

	GdkPaintable *paintable;
	GtkGesture *zoom;
	GtkGesture *drag;

	// State tracking
	float zoom_level;
	float image_x;
	float image_y;

	// Gesture tracking
	float gesture_start_zoom;
	double gesture_image_start_x;
	double gesture_image_start_y;
	double gesture_touch_start_x;
	double gesture_touch_start_y;

	// Cached scaled sizes
	float constrained_x;
	float constrained_y;
	float scaled_width;
	float scaled_height;
};

G_DEFINE_FINAL_TYPE (ExmZoomPicture, exm_zoom_picture, GTK_TYPE_WIDGET)

#define ZOOM_STEP 0.5
#define ZOOM_MIN 0.5
#define ZOOM_MAX 5.0

enum {
	PROP_0,
    PROP_ZOOM,
	N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmZoomPicture *
exm_zoom_picture_new (void)
{
	return g_object_new (EXM_TYPE_ZOOM_PICTURE, NULL);
}

static void
exm_zoom_picture_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
	ExmZoomPicture *self = EXM_ZOOM_PICTURE (object);

	switch (prop_id)
	{
    case PROP_ZOOM:
        g_value_set_float (value, self->zoom_level);
        break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
exm_zoom_picture_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
	ExmZoomPicture *self = EXM_ZOOM_PICTURE (object);

	switch (prop_id)
    {
    case PROP_ZOOM:
        exm_zoom_picture_set_zoom_level (self, g_value_get_float (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

void
exm_zoom_picture_set_paintable (ExmZoomPicture *self,
                                GdkPaintable   *paintable)
{
	if (self->paintable)
		g_object_unref (paintable);

	if (paintable)
		self->paintable = g_object_ref (paintable);

	gtk_widget_queue_draw (GTK_WIDGET (self));
}

GdkPaintable *
exm_zoom_picture_get_paintable (ExmZoomPicture *self)
{
	return self->paintable;
}

void
exm_zoom_picture_set_zoom_level (ExmZoomPicture *self,
								 float           zoom_level)
{
	self->zoom_level = CLAMP (zoom_level, ZOOM_MIN, ZOOM_MAX);

    gtk_widget_queue_draw (GTK_WIDGET (self));
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ZOOM]);
}

float
exm_zoom_picture_get_zoom_level (ExmZoomPicture *self)
{
	return self->zoom_level;
}

float
exm_zoom_picture_get_zoom_level_max (ExmZoomPicture *self)
{
	return ZOOM_MAX;
}

float
exm_zoom_picture_get_zoom_level_min (ExmZoomPicture *self)
{
	return ZOOM_MIN;
}

float
exm_zoom_picture_get_zoom_level_step (ExmZoomPicture *self)
{
	return ZOOM_STEP;
}

void
compute_scaled_dimensions (ExmZoomPicture *self)
{
	float width, height;
	float paintable_width, paintable_height;

	float scale_factor;
	float scaled_width, scaled_height;
	float x, y;

	width = gtk_widget_get_width (GTK_WIDGET (self));
	height = gtk_widget_get_height (GTK_WIDGET (self));

	paintable_width = gdk_paintable_get_intrinsic_width (self->paintable);
	paintable_height = gdk_paintable_get_intrinsic_height (self->paintable);

	// Try fill horizontally
	scale_factor = width / paintable_width;
	scaled_width = paintable_width * scale_factor;
	scaled_height = paintable_height * scale_factor;

	// Doesn't fit - fill vertically
	if (scaled_height > height)
	{
		scale_factor = height / paintable_height;
		scaled_width = paintable_width * scale_factor;
		scaled_height = paintable_height * scale_factor;
	}

	// Scale by zoom level
	scaled_width *= self->zoom_level;
	scaled_height *= self->zoom_level;

	// Set centre to midpoint
	x = ((width - scaled_width) / 2);
	y = ((height - scaled_height) / 2);

	// Constrain to image borders
	self->image_x = CLAMP (self->image_x, x, -x);
	self->image_y = CLAMP (self->image_y, y, -y);

	// Apply offset and constrain to image borders
	if (scaled_width > width)
		x += self->image_x;
	else
		self->image_x = 0.0f;

	if (scaled_height > height)
		y += self->image_y;
	else
		self->image_y = 0.0f;

	// Update for drawing
	self->scaled_width = scaled_width;
	self->scaled_height = scaled_height;
	self->constrained_x = x;
	self->constrained_y = y;
}

void
exm_zoom_picture_snapshot (GtkWidget   *widget,
                           GtkSnapshot *snapshot)
{
	ExmZoomPicture *self = EXM_ZOOM_PICTURE (widget);

	compute_scaled_dimensions (self);

	gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (self->constrained_x, self->constrained_y));
	gdk_paintable_snapshot (self->paintable, snapshot,
							self->scaled_width,
							self->scaled_height);
}

static void
exm_zoom_picture_class_init (ExmZoomPictureClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = exm_zoom_picture_get_property;
	object_class->set_property = exm_zoom_picture_set_property;

    properties [PROP_ZOOM]
        = g_param_spec_float ("zoom-level",
                              "Zoom Level",
                              "Zoom Level",
                              0.0f, G_MAXFLOAT, 0.0f,
                              G_PARAM_CONSTRUCT|G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	widget_class->snapshot = exm_zoom_picture_snapshot;
}

void
on_gesture_begin (GtkGesture       *gesture,
				  GdkEventSequence *sequence,
				  ExmZoomPicture   *self)
{
	gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_CLAIMED);
	self->gesture_start_zoom = self->zoom_level;
	self->gesture_image_start_x = self->image_x;
	self->gesture_image_start_y = self->image_y;

	gtk_gesture_get_bounding_box_center (GTK_GESTURE (gesture),
										 &self->gesture_touch_start_x,
										 &self->gesture_touch_start_y);
}

void
on_scale_changed (GtkGestureZoom *gesture,
				  gdouble         scale,
				  ExmZoomPicture *self)
{
	double gesture_touch_offset_x;
	double gesture_touch_offset_y;
	double delta_x;
	double delta_y;

	exm_zoom_picture_set_zoom_level (self, self->gesture_start_zoom * scale);
	gtk_gesture_get_bounding_box_center(GTK_GESTURE (gesture),
										&gesture_touch_offset_x,
										&gesture_touch_offset_y);

	delta_x = gesture_touch_offset_x - self->gesture_touch_start_x;
	delta_y = gesture_touch_offset_y - self->gesture_touch_start_y;

	self->image_x = self->gesture_image_start_x + delta_x;
	self->image_y = self->gesture_image_start_y + delta_y;

	gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
on_drag_update (GtkGestureDrag *gesture,
                gdouble         offset_x,
                gdouble         offset_y,
                ExmZoomPicture *self)
{
	self->image_x = self->gesture_image_start_x + offset_x;
	self->image_y = self->gesture_image_start_y + offset_y;

	gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
exm_zoom_picture_init (ExmZoomPicture *self)
{
	GtkGesture *gesture;

	exm_zoom_picture_set_zoom_level (self, 1.0f);
	gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);

	gesture = gtk_gesture_zoom_new ();
	gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (gesture), GTK_PHASE_CAPTURE);
	g_signal_connect (gesture, "begin", G_CALLBACK (on_gesture_begin), self);
	g_signal_connect (gesture, "scale-changed", G_CALLBACK (on_scale_changed), self);
	gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (gesture));
	self->zoom = gesture;

	gesture = gtk_gesture_drag_new ();
	gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (gesture), GTK_PHASE_CAPTURE);
	g_signal_connect (gesture, "begin", G_CALLBACK (on_gesture_begin), self);
	g_signal_connect (gesture, "drag-update", G_CALLBACK (on_drag_update), self);
	gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (gesture));
	self->drag = gesture;
}
