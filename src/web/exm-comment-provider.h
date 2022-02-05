#pragma once

#include <glib-object.h>
#include "exm-request-handler.h"

G_BEGIN_DECLS

#define EXM_TYPE_COMMENT_PROVIDER (exm_comment_provider_get_type())

G_DECLARE_FINAL_TYPE (ExmCommentProvider, exm_comment_provider, EXM, COMMENT_PROVIDER, ExmRequestHandler)

ExmCommentProvider *exm_comment_provider_new (void);

void
exm_comment_provider_get_comments_async (ExmCommentProvider  *self,
                                         int                  extension_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data);
GListModel *
exm_comment_provider_get_comments_finish (ExmCommentProvider  *self,
                                          GAsyncResult        *result,
                                          GError             **error);

G_END_DECLS
