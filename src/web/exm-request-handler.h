/*
 * exm-request-handler.h
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

G_BEGIN_DECLS

#define EXM_TYPE_REQUEST_HANDLER (exm_request_handler_get_type())

G_DECLARE_DERIVABLE_TYPE (ExmRequestHandler, exm_request_handler, EXM, REQUEST_HANDLER, GObject)

typedef gpointer (*ResponseHandler)(GBytes *bytes, GError **out_error);

struct _ExmRequestHandlerClass
{
    GObjectClass parent_class;
    ResponseHandler handle_response;
};

ExmRequestHandler *exm_request_handler_new (void);

void
exm_request_handler_request_async (ExmRequestHandler   *self,
                                   const gchar         *url_endpoint,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data);

gpointer
exm_request_handler_request_finish (ExmRequestHandler  *self,
                                    GAsyncResult       *result,
                                    GError            **error);

G_END_DECLS
