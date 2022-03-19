#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EXM_TYPE_RATING (exm_rating_get_type())

G_DECLARE_FINAL_TYPE (ExmRating, exm_rating, EXM, RATING, GtkWidget)

ExmRating *exm_rating_new (void);

G_END_DECLS
