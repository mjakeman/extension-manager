/*
 * exm-version-result.h
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

#define EXM_TYPE_VERSION_RESULT (exm_version_result_get_type())

G_DECLARE_FINAL_TYPE (ExmVersionResult, exm_version_result, EXM, VERSION_RESULT, GObject)

ExmVersionResult *exm_version_result_new                    (void);

gboolean          exm_version_result_supports_shell_version (ExmVersionResult *self,
                                                             const gchar      *shell_version);

G_END_DECLS
