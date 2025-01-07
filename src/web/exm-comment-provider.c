/*
 * exm-comment-provider.c
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

#include "exm-comment-provider.h"

#include "model/exm-comment.h"

#include <json-glib/json-glib.h>

struct _ExmCommentProvider
{
    ExmRequestHandler parent_instance;
};

G_DEFINE_FINAL_TYPE (ExmCommentProvider, exm_comment_provider, EXM_TYPE_REQUEST_HANDLER)

ExmCommentProvider *
exm_comment_provider_new (void)
{
    return g_object_new (EXM_TYPE_COMMENT_PROVIDER, NULL);
}

static GListModel *
parse_comments (GBytes  *bytes,
                GError **out_error)
{
    JsonParser *parser;
    gconstpointer data;
    gsize length;

    GError *error = NULL;
    *out_error = NULL;

    data = g_bytes_get_data (bytes, &length);

    parser = json_parser_new ();
    if (json_parser_load_from_data (parser, data, length, &error))
    {
        // Returned format is an array of objects
        //  - each object is one comment

        GListStore *store = g_list_store_new (EXM_TYPE_COMMENT);

        JsonNode *root = json_parser_get_root (parser);
        g_assert (JSON_NODE_HOLDS_ARRAY (root));

        JsonArray *array = json_node_get_array (root);
        GList *comments = json_array_get_elements (array);

        GList *l;
        for (l = comments; l != NULL; l = l->next)
        {
            GObject *result;

            result = json_gobject_deserialize (EXM_TYPE_COMMENT, l->data);

            g_list_store_append (store, result);
        }

        return G_LIST_MODEL (store);
    }

    *out_error = error;
    return NULL;
}

void
exm_comment_provider_get_comments_async (ExmCommentProvider  *self,
                                         int                  extension_id,
                                         gboolean             retrieve_all,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
    // Query https://extensions.gnome.org/comments/all/?pk={%d}&all=false

    const gchar *all_str = retrieve_all
        ? "true"
        : "false";

    gchar *url;
    url = g_strdup_printf ("https://extensions.gnome.org/comments/all/?pk=%d&all=%s", extension_id, all_str);

    exm_request_handler_request_async (EXM_REQUEST_HANDLER (self),
                                       url,
                                       cancellable,
                                       callback,
                                       user_data);
}

GListModel *
exm_comment_provider_get_comments_finish (ExmCommentProvider  *self,
                                          GAsyncResult        *result,
                                          GError             **error)
{
    gpointer ret;

    ret = exm_request_handler_request_finish (EXM_REQUEST_HANDLER (self),
                                              result,
                                              error);

    return G_LIST_MODEL (ret);
}

static void
exm_comment_provider_class_init (ExmCommentProviderClass *klass)
{
    ExmRequestHandlerClass *request_handler_class = EXM_REQUEST_HANDLER_CLASS (klass);

    request_handler_class->handle_response = (ResponseHandler) parse_comments;
}

static void
exm_comment_provider_init (ExmCommentProvider *self)
{
}
