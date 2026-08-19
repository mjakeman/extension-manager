/*
 * exm-author-page.h
 *
 * Copyright 2022-2026 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#include <adwaita.h>

G_BEGIN_DECLS

#define EXM_TYPE_AUTHOR_PAGE (exm_author_page_get_type())

G_DECLARE_FINAL_TYPE (ExmAuthorPage, exm_author_page, EXM, AUTHOR_PAGE, AdwNavigationPage)

ExmAuthorPage *exm_author_page_new (void);

void           exm_author_page_load (ExmAuthorPage *self,
                                     guint          creator_id,
                                     const gchar   *username,
                                     const gchar   *display_name);

G_END_DECLS
