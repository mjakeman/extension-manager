/*
 * exm-versions-dialog.h
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

#include <adwaita.h>

#include "web/model/exm-shell-versions.h"

G_BEGIN_DECLS

#define EXM_TYPE_VERSIONS_DIALOG (exm_versions_dialog_get_type())

G_DECLARE_FINAL_TYPE (ExmVersionsDialog, exm_versions_dialog, EXM, VERSIONS_DIALOG, AdwDialog)

ExmVersionsDialog *exm_versions_dialog_new                  ();

void               exm_versions_dialog_set_current_version (ExmVersionsDialog *self,
                                                            const gchar       *shell_version);

void               exm_versions_dialog_add_release         (ExmVersionsDialog *self,
                                                            gint               version,
                                                            const gchar       *version_name,
                                                            const gchar       *created,
                                                            ExmShellVersions  *shell_versions);

void               exm_versions_dialog_set_compatible_release (ExmVersionsDialog *self,
                                                               gint               version);

void               exm_versions_dialog_finish              (ExmVersionsDialog *self,
                                                            gboolean           has_more);

G_END_DECLS
