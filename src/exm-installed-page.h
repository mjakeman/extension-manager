/* exm-installed-page.h
 *
 * Copyright 2022-2024 Matthew Jakeman <mjakeman26@outlook.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>

G_BEGIN_DECLS

#define EXM_TYPE_INSTALLED_PAGE (exm_installed_page_get_type())

G_DECLARE_FINAL_TYPE (ExmInstalledPage, exm_installed_page, EXM, INSTALLED_PAGE, GtkWidget)

ExmInstalledPage *
exm_installed_page_new (void);

GtkStack *
exm_installed_page_get_stack (ExmInstalledPage *self);

GtkFilterListModel *
exm_installed_page_get_search_list_model (ExmInstalledPage *self);

G_END_DECLS
