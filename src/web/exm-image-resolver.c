/*
 * exm-image-resolver.c
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

#include "exm-image-resolver.h"

#include "exm-animated-paintable.h"

#include <libsoup/soup.h>
#include <glycin.h>
#include <glycin-gtk4.h>

struct _ExmImageResolver
{
    GObject parent_instance;

    SoupSession *session;
};

G_DEFINE_FINAL_TYPE (ExmImageResolver, exm_image_resolver, G_TYPE_OBJECT)

ExmImageResolver *
exm_image_resolver_new (void)
{
    return g_object_new (EXM_TYPE_IMAGE_RESOLVER, NULL);
}

static void
exm_image_resolver_finalize (GObject *object)
{
    ExmImageResolver *self = (ExmImageResolver *)object;

    g_object_unref (self->session);

    G_OBJECT_CLASS (exm_image_resolver_parent_class)->finalize (object);
}

typedef struct
{
    GTask *task;
    GlyImage *image;
} FrameLoadCtx;

static void
frame_load_ctx_free (FrameLoadCtx *ctx)
{
    g_clear_object (&ctx->image);
    g_free (ctx);
}

static void
on_first_frame_loaded (GObject      *source,
                       GAsyncResult *res,
                       gpointer      user_data)
{
    FrameLoadCtx *ctx = user_data;
    GTask *task = ctx->task;
    GError *error = NULL;
    GlyFrame *frame = gly_image_next_frame_finish (GLY_IMAGE (source), res, &error);

    if (error)
    {
        g_task_return_error (task, error);
        frame_load_ctx_free (ctx);
        g_object_unref (task);
        return;
    }

    GdkTexture *texture = gly_gtk_frame_get_texture (frame);
    gint64 delay_us = gly_frame_get_delay (frame);
    g_object_unref (frame);

    if (delay_us > 0)
    {
        GdkPaintable *paintable = GDK_PAINTABLE (exm_animated_paintable_new (ctx->image, texture, delay_us));
        ctx->image = NULL;
        g_task_return_pointer (task, paintable, g_object_unref);
    }
    else
    {
        g_task_return_pointer (task, texture, g_object_unref);
    }

    frame_load_ctx_free (ctx);
    g_object_unref (task);
}

static void
on_loader_loaded (GObject      *source,
                  GAsyncResult *res,
                  gpointer      user_data)
{
    GTask *task = G_TASK (user_data);
    GError *error = NULL;
    GlyImage *image = gly_loader_load_finish (GLY_LOADER (source), res, &error);
    FrameLoadCtx *ctx;

    if (error)
    {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    ctx = g_new0 (FrameLoadCtx, 1);
    ctx->task = task;
    ctx->image = image;

    gly_image_next_frame_async (image, g_task_get_cancellable (task), on_first_frame_loaded, ctx);
}

static void
image_loaded_callback (GObject      *source,
                       GAsyncResult *res,
                       GTask        *task)
{
    GBytes *bytes;
    GlyLoader *loader;
    GError *error = NULL;

    bytes = soup_session_send_and_read_finish (SOUP_SESSION (source), res, &error);

    if (error)
    {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    loader = gly_loader_new_for_bytes (bytes);
    g_bytes_unref (bytes);

    gly_loader_load_async (loader, g_task_get_cancellable (task), on_loader_loaded, task);
    g_object_unref (loader);
}

void
exm_image_resolver_resolve_async (ExmImageResolver    *self,
                                  const gchar         *rel_path,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
    GTask *task;
    g_autofree gchar *url = NULL;
    SoupMessage *msg;

    task = g_task_new (self, cancellable, callback, user_data);

    if (rel_path == NULL)
    {
        g_task_return_new_error (task, g_quark_from_string ("exm-image-resolver"), -1,
                                 "No image path given");
        g_object_unref (task);
        return;
    }

    // Resolve https://extensions.gnome.org{rel_path}

    url = g_uri_resolve_relative ("https://extensions.gnome.org/",
                                  rel_path,
                                  G_URI_FLAGS_NONE,
                                  NULL);

    if (url != NULL)
    {
        g_autoptr (GUri) parsed = g_uri_parse (url, G_URI_FLAGS_NONE, NULL);
        const gchar *host = parsed ? g_uri_get_host (parsed) : NULL;

        if (g_strcmp0 (host, "extensions.gnome.org") != 0)
        {
            g_task_return_new_error (task, g_quark_from_string ("exm-image-resolver"), -1,
                                     "Refusing to fetch image from unexpected host: %s", url);
            g_object_unref (task);
            return;
        }
    }

    msg = url ? soup_message_new (SOUP_METHOD_GET, url) : NULL;

    if (!msg)
    {
        g_task_return_new_error (task, g_quark_from_string ("exm-image-resolver"), -1,
                                 "Could not construct message for uri: %s", rel_path);
        g_object_unref (task);
        return;
    }

    soup_session_send_and_read_async (self->session, msg,
                                      G_PRIORITY_DEFAULT,
                                      cancellable,
                                      (GAsyncReadyCallback) image_loaded_callback,
                                      task);

    g_object_unref (msg);
}

GdkPaintable *
exm_image_resolver_resolve_finish (ExmImageResolver  *self,
                                   GAsyncResult      *result,
                                   GError           **error)
{
    g_return_val_if_fail (g_task_is_valid (result, self), NULL);

    return g_task_propagate_pointer (G_TASK (result), error);
}

static void
exm_image_resolver_class_init (ExmImageResolverClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_image_resolver_finalize;
}

static void
exm_image_resolver_init (ExmImageResolver *self)
{
    self->session = soup_session_new_with_options ("timeout", 30, NULL);
}
