/*
 * exm-versions-provider.h
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

#include "exm-request-handler.h"

G_BEGIN_DECLS

#define EXM_TYPE_VERSIONS_PROVIDER (exm_versions_provider_get_type())

G_DECLARE_FINAL_TYPE (ExmVersionsProvider, exm_versions_provider, EXM, VERSIONS_PROVIDER, ExmRequestHandler)

ExmVersionsProvider *exm_versions_provider_new              (void);

void                 exm_versions_provider_query_async      (ExmVersionsProvider *self,
                                                             const gchar         *uuid,
                                                             GCancellable        *cancellable,
                                                             GAsyncReadyCallback  callback,
                                                             gpointer             user_data);

void                 exm_versions_provider_query_next_async (ExmVersionsProvider *self,
                                                             gchar               *url,
                                                             GCancellable        *cancellable,
                                                             GAsyncReadyCallback  callback,
                                                             gpointer             user_data);

GListModel          *exm_versions_provider_query_finish     (ExmVersionsProvider  *self,
                                                             GAsyncResult         *result,
                                                             gchar               **next,
                                                             GError              **error);

G_END_DECLS
