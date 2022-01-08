#pragma once

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define EXM_TYPE_SEARCH_PROVIDER (exm_search_provider_get_type())

G_DECLARE_FINAL_TYPE (ExmSearchProvider, exm_search_provider, EXM, SEARCH_PROVIDER, GObject)

ExmSearchProvider *exm_search_provider_new (void);

void
exm_search_provider_query_async (ExmSearchProvider   *self,
                                 const gchar         *query,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data);

GListModel *
exm_search_provider_query_finish (ExmSearchProvider  *self,
                                  GAsyncResult       *result,
                                  GError            **error);

G_END_DECLS
