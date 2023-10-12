#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EXM_TYPE_INFO_BAR (exm_info_bar_get_type())

G_DECLARE_FINAL_TYPE (ExmInfoBar, exm_info_bar, EXM, INFO_BAR, GtkBox)

ExmInfoBar *
exm_info_bar_new (void);

void
exm_info_bar_set_downloads (ExmInfoBar *info_bar, guint downloads);

void
exm_info_bar_set_version (ExmInfoBar *info_bar, double version);

G_END_DECLS
