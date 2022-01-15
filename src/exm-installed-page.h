#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>

G_BEGIN_DECLS

#define EXM_TYPE_INSTALLED_PAGE (exm_installed_page_get_type())

G_DECLARE_FINAL_TYPE (ExmInstalledPage, exm_installed_page, EXM, INSTALLED_PAGE, GtkWidget)

ExmInstalledPage *exm_installed_page_new (void);

G_END_DECLS
