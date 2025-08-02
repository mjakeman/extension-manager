/*
 * exm-data-provider.h
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

#include "exm-request-handler.h"

#include "model/exm-search-result.h"

G_BEGIN_DECLS

#define EXM_TYPE_DATA_PROVIDER (exm_data_provider_get_type())

G_DECLARE_FINAL_TYPE (ExmDataProvider, exm_data_provider, EXM, DATA_PROVIDER, ExmRequestHandler)

ExmDataProvider *exm_data_provider_new (void);

void
exm_data_provider_get_async (ExmDataProvider     *self,
                             const gchar         *uuid,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data);

ExmSearchResult *
exm_data_provider_get_finish (ExmDataProvider  *self,
                              GAsyncResult     *result,
                              GError          **error);

G_END_DECLS
