# pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define EXM_TYPE_SCREENSHOT_VIEW (exm_screenshot_view_get_type())

G_DECLARE_FINAL_TYPE (ExmScreenshotView, exm_screenshot_view, EXM, SCREENSHOT_VIEW, AdwNavigationPage)

ExmScreenshotView *
exm_screenshot_view_new (void);

void
exm_screenshot_view_zoom (ExmScreenshotView *self,
                          const char        *action_name);

G_END_DECLS
