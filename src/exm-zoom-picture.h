/* exm-zoom-picture.h
 *
 * Copyright 2022 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EXM_TYPE_ZOOM_PICTURE (exm_zoom_picture_get_type())

G_DECLARE_FINAL_TYPE (ExmZoomPicture, exm_zoom_picture, EXM, ZOOM_PICTURE, GtkWidget)

ExmZoomPicture *exm_zoom_picture_new (void);

void
exm_zoom_picture_set_paintable (ExmZoomPicture *self,
                                GdkPaintable   *paintable);

GdkPaintable *
exm_zoom_picture_get_paintable (ExmZoomPicture *self);

G_END_DECLS
