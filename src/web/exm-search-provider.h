#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define EXM_TYPE_SEARCH_PROVIDER (exm_search_provider_get_type())

G_DECLARE_FINAL_TYPE (ExmSearchProvider, exm_search_provider, EXM, SEARCH_PROVIDER, GObject)

ExmSearchProvider *exm_search_provider_new (void);

void exm_search_provider_query (ExmSearchProvider *provider,
                                const gchar       *query);

G_END_DECLS
