/*
 * exm-shell-versions.h
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

#define EXM_TYPE_SHELL_VERSIONS (exm_shell_versions_get_type ())

typedef struct _ExmShellVersions ExmShellVersions;

typedef struct
{
    gint major;
    gint minor;
    gint point;
} Entry;

struct _ExmShellVersions
{
    /*< private >*/
    guint ref_count;
    GList *versions;

};

GType             exm_shell_versions_get_type (void) G_GNUC_CONST;
ExmShellVersions *exm_shell_versions_new      (void);
ExmShellVersions *exm_shell_versions_ref      (ExmShellVersions *self);
void              exm_shell_versions_unref    (ExmShellVersions *self);
void              exm_shell_versions_add      (ExmShellVersions *self,
                                               gint              major,
                                               gint              minor,
                                               gint              point);
gboolean          exm_shell_versions_supports (ExmShellVersions *self,
                                               const gchar      *shell_version);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ExmShellVersions, exm_shell_versions_unref)

G_END_DECLS
