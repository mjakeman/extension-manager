#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define EXM_TYPE_SCREENSHOT (exm_screenshot_get_type())

G_DECLARE_FINAL_TYPE (ExmScreenshot, exm_screenshot, EXM, SCREENSHOT, GtkWidget)

ExmScreenshot *exm_screenshot_new (void);
void exm_screenshot_set_paintable (ExmScreenshot *self,
                                   GdkPaintable  *paintable);
void exm_screenshot_reset         (ExmScreenshot *self);
void exm_screenshot_display       (ExmScreenshot *self);

G_END_DECLS
