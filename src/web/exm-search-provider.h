#pragma once

#include <glib-object.h>
#include <gio/gio.h>

#include "exm-request-handler.h"

G_BEGIN_DECLS

#define EXM_TYPE_SEARCH_PROVIDER (exm_search_provider_get_type())

G_DECLARE_FINAL_TYPE (ExmSearchProvider, exm_search_provider, EXM, SEARCH_PROVIDER, ExmRequestHandler)

typedef enum
{
    EXM_SEARCH_SORT_POPULARITY_DES = 0,
    EXM_SEARCH_SORT_CREATED_DES = 1,
    EXM_SEARCH_SORT_DOWNLOADS_DES = 2,
    EXM_SEARCH_SORT_UPDATED_DES = 3,
    EXM_SEARCH_SORT_POPULARITY_ASC = 4,
    EXM_SEARCH_SORT_CREATED_ASC = 5,
    EXM_SEARCH_SORT_DOWNLOADS_ASC = 6,
    EXM_SEARCH_SORT_UPDATED_ASC = 7
} ExmSearchSort;

ExmSearchProvider *exm_search_provider_new (void);

void
exm_search_provider_query_async (ExmSearchProvider   *self,
                                 const gchar         *query,
                                 int                  page,
                                 ExmSearchSort        sort_type,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data);

GListModel *
exm_search_provider_query_finish (ExmSearchProvider  *self,
                                  GAsyncResult       *result,
                                  int                *num_pages,
                                  GError            **error);

G_END_DECLS
