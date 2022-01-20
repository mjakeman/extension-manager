#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define EXM_TYPE_SEARCH_RESULT (exm_search_result_get_type())

G_DECLARE_FINAL_TYPE (ExmSearchResult, exm_search_result, EXM, SEARCH_RESULT, GObject)

ExmSearchResult *exm_search_result_new (void);

gboolean exm_search_result_supports_shell_version (ExmSearchResult *self,
                                                   const gchar     *shell_version);

G_END_DECLS
