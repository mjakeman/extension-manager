#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>

G_BEGIN_DECLS

#define EXM_TYPE_BROWSE_PAGE (exm_browse_page_get_type())

G_DECLARE_FINAL_TYPE (ExmBrowsePage, exm_browse_page, EXM, BROWSE_PAGE, GtkWidget)

ExmBrowsePage *exm_browse_page_new (void);
void exm_browse_page_refresh (ExmBrowsePage *self);

G_END_DECLS
