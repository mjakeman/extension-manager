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
#include <glib/gi18n.h>
#include <libsoup/soup.h>

typedef struct
{
    SoupSession *session;

    GHashTable *cache;
} ExmRequestHandlerPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ExmRequestHandler, exm_request_handler, G_TYPE_OBJECT)

typedef struct
{
    GBytes *bytes;
    gint64  expiry_us;
} CacheEntry;

static void
cache_entry_free (CacheEntry *entry)
{
    g_bytes_unref (entry->bytes);
    g_free (entry);
}

static void
purge_expired_cache_entries (GHashTable *cache)
{
    GHashTableIter iter;
    gpointer key, value;
    gint64 now = g_get_monotonic_time ();

    g_hash_table_iter_init (&iter, cache);
    while (g_hash_table_iter_next (&iter, &key, &value))
    {
        CacheEntry *entry = value;

        if (entry->expiry_us <= now)
            g_hash_table_iter_remove (&iter);
    }
}

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

SoupSession *
exm_request_handler_get_shared_session (void)
{
    static SoupSession *shared = NULL;

    if (g_once_init_enter (&shared))
    {
        SoupSession *session = soup_session_new_with_options ("timeout", 30, NULL);
        g_once_init_leave (&shared, session);
    }

    return shared;
}

static void
exm_request_handler_finalize (GObject *object)
{
    ExmRequestHandlerPrivate *priv = exm_request_handler_get_instance_private ((ExmRequestHandler *)object);

    g_hash_table_unref (priv->cache);

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
    gchar *url;
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
    else if (status_code >= SOUP_STATUS_BAD_REQUEST)
    {
        g_task_return_new_error (data->task,
                                 g_quark_from_string ("request-error-quark"),
                                 status_code,
                                 status_code >= SOUP_STATUS_INTERNAL_SERVER_ERROR
                                    ? _("Check <a href='https://status.gnome.org/'>GNOME infrastructure status</a> and try again later")
                                    : _("Check your network status and try again"));
    }
    else
    {
        ExmRequestHandlerClass *klass = EXM_REQUEST_HANDLER_CLASS (G_OBJECT_GET_CLASS (data->self));

        if (klass->cache_ttl_seconds > 0)
        {
            ExmRequestHandlerPrivate *priv = exm_request_handler_get_instance_private (data->self);
            CacheEntry *entry = g_new0 (CacheEntry, 1);

            entry->bytes = g_bytes_ref (bytes);
            entry->expiry_us = g_get_monotonic_time () +
                                ((gint64) klass->cache_ttl_seconds * G_USEC_PER_SEC);

            g_hash_table_insert (priv->cache, g_strdup (data->url), entry);
        }

        // Get derived class to handle response
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
    g_free (data->url);
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
    ExmRequestHandlerClass *klass;

    GTask *task;
    SoupMessage *msg;

    priv = exm_request_handler_get_instance_private (self);
    klass = EXM_REQUEST_HANDLER_CLASS (G_OBJECT_GET_CLASS (self));

    task = g_task_new (self, cancellable, callback, user_data);

    if (klass->cache_ttl_seconds > 0)
    {
        CacheEntry *entry;

        purge_expired_cache_entries (priv->cache);
        entry = g_hash_table_lookup (priv->cache, url_endpoint);

        if (entry != NULL)
        {
            GError *error = NULL;
            gpointer result = klass->handle_response (entry->bytes, &error);

            if (result == NULL)
                g_task_return_error (task, error);
            else
                g_task_return_pointer (task, result, g_object_unref);

            g_object_unref (task);
            return;
        }
    }

    msg = soup_message_new (SOUP_METHOD_GET, url_endpoint);

    if (!msg)
    {
        g_task_return_new_error (task, g_quark_from_string ("exm-search-provider"), -1,
                                 "Could not construct message for uri: %s", url_endpoint);
        g_object_unref (task);
        return;
    }

    RequestData *data = g_new0 (RequestData, 1);
    data->self = g_object_ref (self);
    data->task = g_object_ref (task);
    data->msg = g_object_ref (msg);
    data->url = g_strdup (url_endpoint);

    soup_session_send_and_read_async (priv->session, msg,
                                      G_PRIORITY_DEFAULT,
                                      cancellable,
                                      (GAsyncReadyCallback) request_callback,
                                      data);

    g_object_unref (msg);
    g_object_unref (task);
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
    priv->session = exm_request_handler_get_shared_session ();
    priv->cache = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         g_free, (GDestroyNotify) cache_entry_free);
}
