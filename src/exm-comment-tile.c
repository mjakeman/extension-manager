/* exm-comment-tile.c
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

#include "exm-comment-tile.h"

#include "exm-rating.h"

#include "exm-config.h"

struct _ExmCommentTile
{
    GtkWidget parent_instance;

    ExmComment *comment;

    GtkLabel *author;
    GtkLabel *author_badge;
    ExmRating *rating;
    GtkLabel *date;
};

G_DEFINE_FINAL_TYPE (ExmCommentTile, exm_comment_tile, GTK_TYPE_WIDGET)

enum {
    PROP_0,
    PROP_COMMENT,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmCommentTile *
exm_comment_tile_new (ExmComment *comment)
{
    return g_object_new (EXM_TYPE_COMMENT_TILE,
                         "comment", comment,
                         NULL);
}

static void
exm_comment_tile_finalize (GObject *object)
{
    GtkWidget *child;
    ExmCommentTile *self = (ExmCommentTile *)object;

    child = gtk_widget_get_first_child (GTK_WIDGET (self));
    gtk_widget_unparent (child);

    G_OBJECT_CLASS (exm_comment_tile_parent_class)->finalize (object);
}

static void
exm_comment_tile_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
    ExmCommentTile *self = EXM_COMMENT_TILE (object);

    switch (prop_id)
    {
    case PROP_COMMENT:
        g_value_set_object (value, self->comment);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_comment_tile_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
    ExmCommentTile *self = EXM_COMMENT_TILE (object);

    switch (prop_id)
    {
    case PROP_COMMENT:
        self->comment = g_value_get_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_comment_tile_constructed (GObject *object)
{
    ExmCommentTile *self = EXM_COMMENT_TILE (object);

    g_return_if_fail (EXM_IS_COMMENT (self->comment));

    GDateTime *datetime;

    gchar *date;
    g_object_get (self->comment,
                  "date", &date,
                  NULL);

    datetime = g_date_time_new_from_iso8601 (date, g_time_zone_new_utc ());
    g_free (date);

    if (datetime != NULL)
    {
      gtk_label_set_label (self->date, g_date_time_format (datetime, "%x"));
      g_date_time_unref (datetime);
    }

    G_OBJECT_CLASS (exm_comment_tile_parent_class)->constructed (object);
}

static void
exm_comment_tile_class_init (ExmCommentTileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_comment_tile_finalize;
    object_class->get_property = exm_comment_tile_get_property;
    object_class->set_property = exm_comment_tile_set_property;
    object_class->constructed = exm_comment_tile_constructed;

    properties [PROP_COMMENT] =
        g_param_spec_object ("comment",
                             "Comment",
                             "Comment",
                             EXM_TYPE_COMMENT,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-comment-tile.ui", RESOURCE_PATH));

    gtk_widget_class_bind_template_child (widget_class, ExmCommentTile, author);
    gtk_widget_class_bind_template_child (widget_class, ExmCommentTile, author_badge);
    gtk_widget_class_bind_template_child (widget_class, ExmCommentTile, rating);
    gtk_widget_class_bind_template_child (widget_class, ExmCommentTile, date);

    gtk_widget_class_set_css_name (widget_class, "comment-tile");
    gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
exm_comment_tile_init (ExmCommentTile *self)
{
    g_type_ensure (EXM_TYPE_RATING);

    gtk_widget_init_template (GTK_WIDGET (self));
}
