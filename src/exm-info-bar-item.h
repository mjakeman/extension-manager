#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define EXM_TYPE_INFO_BAR_ITEM (exm_info_bar_item_get_type())

G_DECLARE_FINAL_TYPE (ExmInfoBarItem, exm_info_bar_item, EXM, INFO_BAR_ITEM, AdwBin)

ExmInfoBarItem *
exm_info_bar_item_new (void);

void
exm_info_bar_item_set_subtitle (ExmInfoBarItem *info_bar_item, const char *subtitle);

G_END_DECLS
