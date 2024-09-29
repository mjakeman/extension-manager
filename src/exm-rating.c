/* exm-rating.c
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

#include "exm-rating.h"

#include <glib/gi18n.h>

struct _ExmRating
{
    GtkWidget parent_instance;

    int rating;

    GtkImage *star_one;
    GtkImage *star_two;
    GtkImage *star_three;
    GtkImage *star_four;
    GtkImage *star_five;
};

G_DEFINE_FINAL_TYPE (ExmRating, exm_rating, GTK_TYPE_WIDGET)

enum {
    PROP_0,
    PROP_RATING,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmRating *
exm_rating_new (void)
{
    return g_object_new (EXM_TYPE_RATING, NULL);
}

static void
exm_rating_finalize (GObject *object)
{
    GtkWidget *child;
    ExmRating *self = (ExmRating *)object;

    child = gtk_widget_get_first_child (GTK_WIDGET (self));
    gtk_widget_unparent (child);

    G_OBJECT_CLASS (exm_rating_parent_class)->finalize (object);
}

void update_rating (ExmRating *self);

static void
exm_rating_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
    ExmRating *self = EXM_RATING (object);

    switch (prop_id)
    {
    case PROP_RATING:
        g_value_set_int (value, self->rating);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_rating_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
    ExmRating *self = EXM_RATING (object);

    switch (prop_id)
    {
    case PROP_RATING:
        self->rating = g_value_get_int (value);
        update_rating (self);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

#define FILLED_ICON_NAME "star-filled-rounded-symbolic"
#define EMPTY_ICON_NAME "star-outline-rounded-symbolic"

void update_rating (ExmRating *self)
{
    gtk_image_set_from_icon_name (self->star_one, EMPTY_ICON_NAME);
    gtk_image_set_from_icon_name (self->star_two, EMPTY_ICON_NAME);
    gtk_image_set_from_icon_name (self->star_three, EMPTY_ICON_NAME);
    gtk_image_set_from_icon_name (self->star_four, EMPTY_ICON_NAME);
    gtk_image_set_from_icon_name (self->star_five, EMPTY_ICON_NAME);

    switch (self->rating)
    {
    case 5:
        gtk_image_set_from_icon_name (self->star_five, FILLED_ICON_NAME);
    case 4:
        gtk_image_set_from_icon_name (self->star_four, FILLED_ICON_NAME);
    case 3:
        gtk_image_set_from_icon_name (self->star_three, FILLED_ICON_NAME);
    case 2:
        gtk_image_set_from_icon_name (self->star_two, FILLED_ICON_NAME);
    case 1:
        gtk_image_set_from_icon_name (self->star_one, FILLED_ICON_NAME);
    case 0:
    default:
        break;
    }

    gtk_widget_set_tooltip_text (GTK_WIDGET (self),
                                 g_strdup_printf (ngettext ("%i Star", "%i Stars", self->rating),
                                                  self->rating));
}

static void
exm_rating_class_init (ExmRatingClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_rating_finalize;
    object_class->get_property = exm_rating_get_property;
    object_class->set_property = exm_rating_set_property;

    properties [PROP_RATING]
        = g_param_spec_int ("rating",
                            "Rating",
                            "Rating",
                            0, G_MAXINT, 0,
                            G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-rating.ui");

    gtk_widget_class_bind_template_child (widget_class, ExmRating, star_one);
    gtk_widget_class_bind_template_child (widget_class, ExmRating, star_two);
    gtk_widget_class_bind_template_child (widget_class, ExmRating, star_three);
    gtk_widget_class_bind_template_child (widget_class, ExmRating, star_four);
    gtk_widget_class_bind_template_child (widget_class, ExmRating, star_five);

    gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
    gtk_widget_class_set_css_name (widget_class, "rating");
}

static void
exm_rating_init (ExmRating *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    update_rating (self);
}
