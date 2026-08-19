/*
 * exm-profile-provider.h
 *
 * Copyright 2022-2026 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#include "model/exm-user-profile.h"

G_BEGIN_DECLS

#define EXM_TYPE_PROFILE_PROVIDER (exm_profile_provider_get_type())

G_DECLARE_FINAL_TYPE (ExmProfileProvider, exm_profile_provider, EXM, PROFILE_PROVIDER, ExmRequestHandler)

ExmProfileProvider *exm_profile_provider_new (void);

void
exm_profile_provider_get_async (ExmProfileProvider  *self,
                                guint                user_id,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data);

ExmUserProfile *
exm_profile_provider_get_finish (ExmProfileProvider  *self,
                                 GAsyncResult        *result,
                                 GError             **error);

G_END_DECLS
