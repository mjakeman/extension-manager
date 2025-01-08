/*
 * exm-request-handler.c
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

#include "exm-request-handler.h"

#include <gio/gio.h>
#include <libsoup/soup.h>

typedef struct
{
    SoupSession *session;
} ExmRequestHandlerPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ExmRequestHandler, exm_request_handler, G_TYPE_OBJECT)

/**
 * exm_request_handler_new:
 *
 * Create a new #ExmRequestHandler.
 *
 * Returns: (transfer full): a newly created #ExmRequestHandler
 */
ExmRequestHandler *
exm_request_handler_new (void)
{
    return g_object_new (EXM_TYPE_REQUEST_HANDLER, NULL);
}

static void
exm_request_handler_finalize (GObject *object)
{
    ExmRequestHandler *self = (ExmRequestHandler *)object;
    ExmRequestHandlerPrivate *priv = exm_request_handler_get_instance_private (self);

    g_object_unref (priv->session);

    G_OBJECT_CLASS (exm_request_handler_parent_class)->finalize (object);
}

static void *
default_handle_response (GBytes  *bytes G_GNUC_UNUSED,
                         GError **out_error)
{
    *out_error = NULL;
    g_warning ("You have not overridden the `handle_response` virtual method for subclass of ExmRequestHandler. Nothing will happen.\n");
    return NULL;
}

typedef struct
{
    ExmRequestHandler *self;
    GTask *task;
    SoupMessage *msg;
} RequestData;

static void
request_callback (GObject      *source,
                  GAsyncResult *res,
                  RequestData  *data)
{
    GBytes *bytes;
    GListModel *model;
    guint status_code;
    GError *error = NULL;

    bytes = soup_session_send_and_read_finish (SOUP_SESSION (source), res, &error);

    g_object_get (G_OBJECT (data->msg), "status-code", &status_code, NULL);

    if (error)
    {
        g_task_return_error (data->task, error);
    }
    else if (status_code != SOUP_STATUS_OK)
    {
        g_task_return_new_error (data->task, g_quark_from_string ("exm-request-handler"), status_code,
                                 "HTTP error: %d", status_code);
    }
    else
    {
        // Get derived class to handle response
        ExmRequestHandlerClass *klass = EXM_REQUEST_HANDLER_CLASS (G_OBJECT_GET_CLASS (data->self));
        model = klass->handle_response (bytes, &error);

        if (model == NULL)
            g_task_return_error (data->task, error);
        else
            g_task_return_pointer (data->task, model, g_object_unref);

        g_bytes_unref (bytes);
    }

    g_object_unref (data->self);
    g_object_unref (data->task);
    g_object_unref (data->msg);
    g_free (data);
}

void
exm_request_handler_request_async (ExmRequestHandler   *self,
                                   const gchar         *url_endpoint,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
    ExmRequestHandlerPrivate *priv;

    GTask *task;
    SoupMessage *msg;

    priv = exm_request_handler_get_instance_private (self);

    task = g_task_new (self, cancellable, callback, user_data);
    msg = soup_message_new (SOUP_METHOD_GET, url_endpoint);

    if (!msg)
    {
        g_task_return_new_error (task, g_quark_from_string ("exm-search-provider"), -1,
                                 "Could not construct message for uri: %s", url_endpoint);
        return;
    }

    RequestData *data = g_new0 (RequestData, 1);
    data->self = g_object_ref (self);
    data->task = g_object_ref (task);
    data->msg = g_object_ref (msg);

    soup_session_send_and_read_async (priv->session, msg,
                                      G_PRIORITY_DEFAULT,
                                      cancellable,
                                      (GAsyncReadyCallback) request_callback,
                                      data);

    g_object_unref (msg);
}

gpointer
exm_request_handler_request_finish (ExmRequestHandler  *self,
                                    GAsyncResult       *result,
                                    GError            **error)
{
    g_return_val_if_fail (g_task_is_valid (result, self), NULL);

    return g_task_propagate_pointer (G_TASK (result), error);
}

static void
exm_request_handler_class_init (ExmRequestHandlerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_request_handler_finalize;

    ExmRequestHandlerClass *request_handler_class = EXM_REQUEST_HANDLER_CLASS (klass);

    request_handler_class->handle_response = default_handle_response;
}

static void
exm_request_handler_init (ExmRequestHandler *self)
{
    ExmRequestHandlerPrivate *priv = exm_request_handler_get_instance_private (self);
    priv->session = soup_session_new ();
}
