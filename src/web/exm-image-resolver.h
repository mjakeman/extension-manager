/*
 * exm-image-resolver.h
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

#pragma once

#include <glib-object.h>
#include <gio/gio.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define EXM_TYPE_IMAGE_RESOLVER (exm_image_resolver_get_type())

G_DECLARE_FINAL_TYPE (ExmImageResolver, exm_image_resolver, EXM, IMAGE_RESOLVER, GObject)

ExmImageResolver *exm_image_resolver_new (void);
void exm_image_resolver_resolve_async (ExmImageResolver    *self,
                                       const gchar         *rel_path,
                                       GCancellable        *cancellable,
                                       GAsyncReadyCallback  callback,
                                       gpointer             user_data);
GdkTexture *exm_image_resolver_resolve_finish (ExmImageResolver  *self,
                                               GAsyncResult      *result,
                                               GError           **error);

G_END_DECLS
