#pragma once

#include <glib-object.h>

#include "exm-request-handler.h"

#include "model/exm-search-result.h"

G_BEGIN_DECLS

#define EXM_TYPE_DATA_PROVIDER (exm_data_provider_get_type())

G_DECLARE_FINAL_TYPE (ExmDataProvider, exm_data_provider, EXM, DATA_PROVIDER, ExmRequestHandler)

ExmDataProvider *exm_data_provider_new (void);

void
exm_data_provider_get_async (ExmDataProvider     *self,
                             const gchar         *uuid,
                             int                  pk,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data);

ExmSearchResult *
exm_data_provider_get_finish (ExmDataProvider  *self,
                              GAsyncResult     *result,
                              GError          **error);

G_END_DECLS
