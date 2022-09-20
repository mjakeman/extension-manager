#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define EXM_TYPE_DETAIL_VIEW (exm_detail_view_get_type())

G_DECLARE_FINAL_TYPE (ExmDetailView, exm_detail_view, EXM, DETAIL_VIEW, AdwBin)

ExmDetailView *exm_detail_view_new (void);

void
exm_detail_view_load_for_uuid (ExmDetailView *self,
                               gchar         *uuid);

void
exm_detail_view_update (ExmDetailView *self);

G_END_DECLS
