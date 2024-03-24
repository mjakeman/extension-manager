/* exm-unified-data.h
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

#include <glib-object.h>

#include "web/model/exm-search-result.h"
#include "web/model/exm-shell-version-map.h"
#include "local/exm-extension.h"

G_BEGIN_DECLS

#define EXM_TYPE_UNIFIED_DATA (exm_unified_data_get_type())

G_DECLARE_FINAL_TYPE (ExmUnifiedData, exm_unified_data, EXM, UNIFIED_DATA, GObject)

ExmUnifiedData *exm_unified_data_new (void);

ExmExtension *
exm_unified_data_get_local_data (ExmUnifiedData *self);

void
exm_unified_data_set_local_data (ExmUnifiedData *self,
                                 ExmExtension   *extension);

ExmSearchResult *
exm_unified_data_get_web_data (ExmUnifiedData *self);

void
exm_unified_data_set_web_data (ExmUnifiedData  *self,
                               ExmSearchResult *extension);

const char *
exm_unified_data_get_name (ExmUnifiedData *self);

const char *
exm_unified_data_get_creator (ExmUnifiedData *self);

const char *
exm_unified_data_get_uuid (ExmUnifiedData *self);

const char *
exm_unified_data_get_description (ExmUnifiedData *self);

gboolean
exm_unified_data_get_screenshot_uri (ExmUnifiedData  *self,
                                     char           **uri);

gboolean
exm_unified_data_get_link (ExmUnifiedData  *self,
                           char           **link);

gboolean
exm_unified_data_get_homepage (ExmUnifiedData  *self,
                               char           **homepage);

gboolean
exm_unified_data_get_pk (ExmUnifiedData *self,
                         int            *pk);

gboolean
exm_unified_data_is_empty (ExmUnifiedData *self);

ExmShellVersionMap *
exm_unified_data_get_shell_version_map (ExmUnifiedData *self);

G_END_DECLS
