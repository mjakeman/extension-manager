/*
 * exm-manager.h
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

#include "exm-extension.h"

G_BEGIN_DECLS

#define EXM_TYPE_MANAGER (exm_manager_get_type())

G_DECLARE_FINAL_TYPE (ExmManager, exm_manager, EXM, MANAGER, GObject)

ExmManager   *exm_manager_new (void);
void          exm_manager_enable_extension  (ExmManager           *manager,
                                             ExmExtension         *extension);
void          exm_manager_disable_extension (ExmManager           *manager,
                                             ExmExtension         *extension);
void          exm_manager_remove_extension  (ExmManager           *self,
                                             ExmExtension         *extension);
void          exm_manager_open_prefs        (ExmManager           *self,
                                             ExmExtension         *extension);
void          exm_manager_check_for_updates (ExmManager           *self);
gboolean      exm_manager_is_installed_uuid (ExmManager           *self,
                                             const gchar          *uuid);
ExmExtension *exm_manager_get_by_uuid       (ExmManager           *self,
                                             const gchar          *uuid);
void          exm_manager_install_async     (ExmManager           *self,
                                             const gchar          *uuid,
                                             GCancellable         *cancellable,
                                             GAsyncReadyCallback   callback,
                                             gpointer              user_data);
gboolean      exm_manager_install_finish    (GObject              *self,
                                             GAsyncResult         *result,
                                             GError              **error);

G_END_DECLS
