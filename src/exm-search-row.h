#pragma once

#include <adwaita.h>

#include "web/model/exm-search-result.h"

G_BEGIN_DECLS

#define EXM_TYPE_SEARCH_ROW (exm_search_row_get_type())

G_DECLARE_FINAL_TYPE (ExmSearchRow, exm_search_row, EXM, SEARCH_ROW, AdwExpanderRow)

ExmSearchRow *exm_search_row_new (ExmSearchResult *search_result,
                                  gboolean         is_installed);

G_END_DECLS
