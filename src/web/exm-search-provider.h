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

#define EXM_SEARCH_PROVIDER_MAX_UUIDS_PER_REQUEST 50

G_BEGIN_DECLS

#define EXM_TYPE_SEARCH_PROVIDER (exm_search_provider_get_type())

G_DECLARE_FINAL_TYPE (ExmSearchProvider, exm_search_provider, EXM, SEARCH_PROVIDER, ExmRequestHandler)

typedef enum
{
    EXM_SEARCH_SORT_NONE = 0,
    EXM_SEARCH_SORT_POPULARITY_DES = 1,
    EXM_SEARCH_SORT_DOWNLOADS_DES = 2,
    EXM_SEARCH_SORT_UPDATED_DES = 3,
    EXM_SEARCH_SORT_CREATED_DES = 4,
    EXM_SEARCH_SORT_POPULARITY_ASC = 5,
    EXM_SEARCH_SORT_DOWNLOADS_ASC = 6,
    EXM_SEARCH_SORT_UPDATED_ASC = 7,
    EXM_SEARCH_SORT_CREATED_ASC = 8
} ExmSearchSort;

ExmSearchProvider *exm_search_provider_new          (void);

void               exm_search_provider_query_async  (ExmSearchProvider   *self,
                                                     const gchar         *query,
                                                     int                  page,
                                                     int                  page_size,
                                                     ExmSearchSort        sort_type,
                                                     gboolean             only_supported,
                                                     GCancellable        *cancellable,
                                                     GAsyncReadyCallback  callback,
                                                     gpointer             user_data);

GListModel        *exm_search_provider_query_finish (ExmSearchProvider  *self,
                                                     GAsyncResult       *result,
                                                     int                *num_pages,
                                                     GError            **error);

void               exm_search_provider_mark_unsupported (ExmSearchProvider *self,
                                                         GListModel        *results,
                                                         guint              start,
                                                         GCancellable      *cancellable);

gboolean           exm_search_provider_query_uuids_async (ExmSearchProvider    *self,
                                                          const gchar * const  *uuids,
                                                          guint                 n_uuids,
                                                          const gchar          *shell_version,
                                                          GCancellable         *cancellable,
                                                          GAsyncReadyCallback   callback,
                                                          gpointer              user_data);

void                exm_search_provider_query_by_creator_async (ExmSearchProvider   *self,
                                                                const gchar         *username,
                                                                GCancellable        *cancellable,
                                                                GAsyncReadyCallback  callback,
                                                                gpointer             user_data);

G_END_DECLS
