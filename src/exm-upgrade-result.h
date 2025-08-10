/*
 * exm-upgrade-result.h
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

#include "web/model/exm-version-result.h"
#include "local/exm-extension.h"

G_BEGIN_DECLS

#define EXM_TYPE_UPGRADE_RESULT (exm_upgrade_result_get_type())

G_DECLARE_FINAL_TYPE (ExmUpgradeResult, exm_upgrade_result, EXM, UPGRADE_RESULT, GObject)

ExmUpgradeResult *exm_upgrade_result_new            (void);

ExmExtension     *exm_upgrade_result_get_local_data (ExmUpgradeResult *self);

void              exm_upgrade_result_set_local_data (ExmUpgradeResult *self,
                                                     ExmExtension     *extension);

ExmVersionResult *exm_upgrade_result_get_web_data   (ExmUpgradeResult *self);

void              exm_upgrade_result_set_web_data   (ExmUpgradeResult *self,
                                                     ExmVersionResult *extension);

const char       *exm_upgrade_result_get_name       (ExmUpgradeResult *self);

const char       *exm_upgrade_result_get_creator    (ExmUpgradeResult *self);

const char       *exm_upgrade_result_get_uuid       (ExmUpgradeResult *self);

G_END_DECLS
