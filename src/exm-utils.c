/* exm-utils.c
 *
 * Copyright 2022 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#include "exm-utils.h"

char *
exm_utils_read_resource (const char *resource, gsize *length)
{
  GError *error = NULL;

  GFile *file;
  char *path;
  char *contents;

  g_return_val_if_fail (length != NULL, NULL);
  g_return_val_if_fail (resource != NULL, NULL);

  path = g_strdup_printf ("resource://%s", resource);
  file = g_file_new_for_uri (path);
  g_free (path);

  if (!file)
  {
      g_critical ("Could not read %s: invalid file\n", resource);
      return NULL;
  }

  if (g_file_load_contents (file, NULL, &contents, length, NULL, &error))
  {
      g_clear_object (&file);
      return contents;
  }
  else
  {
      g_critical ("Could not read %s: %s\n", resource, error->message);
  }

  g_clear_object (&file);
  return NULL;
}
