/*
 * exm-search-provider.h
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

#define EXM_TYPE_SEARCH_PROVIDER (exm_search_provider_get_type())

G_DECLARE_FINAL_TYPE (ExmSearchProvider, exm_search_provider, EXM, SEARCH_PROVIDER, ExmRequestHandler)

typedef enum
{
    EXM_SEARCH_SORT_DOWNLOADS_DES = 0,
    EXM_SEARCH_SORT_POPULARITY_DES = 1,
    EXM_SEARCH_SORT_CREATED_DES = 2,
    EXM_SEARCH_SORT_UPDATED_DES = 3,
    EXM_SEARCH_SORT_DOWNLOADS_ASC = 4,
    EXM_SEARCH_SORT_POPULARITY_ASC = 5,
    EXM_SEARCH_SORT_CREATED_ASC = 6,
    EXM_SEARCH_SORT_UPDATED_ASC = 7
} ExmSearchSort;

ExmSearchProvider *exm_search_provider_new          (void);

void               exm_search_provider_query_async  (ExmSearchProvider   *self,
                                                     const gchar         *query,
                                                     int                  page,
                                                     ExmSearchSort        sort_type,
                                                     GCancellable        *cancellable,
                                                     GAsyncReadyCallback  callback,
                                                     gpointer             user_data);

GListModel        *exm_search_provider_query_finish (ExmSearchProvider  *self,
                                                     GAsyncResult       *result,
                                                     int                *num_pages,
                                                     GError            **error);

G_END_DECLS
