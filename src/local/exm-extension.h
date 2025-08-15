/*
 * exm-extension.h
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

G_BEGIN_DECLS

#define EXM_TYPE_EXTENSION (exm_extension_get_type())

G_DECLARE_FINAL_TYPE (ExmExtension, exm_extension, EXM, EXTENSION, GObject)

ExmExtension *exm_extension_new  (const gchar *uuid);

gint          compare_extension  (ExmExtension *a,
                                  ExmExtension *b,
                                  gpointer      user_data);

gboolean      is_extension_equal (ExmExtension *a,
                                  ExmExtension *b);

G_END_DECLS
