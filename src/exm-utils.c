/* exm-utils.c
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

#include "exm-utils.h"

#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>

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

static GString *
build_text_recursive (xmlNode *node, GString *string)
{
    xmlNode *cur_node = NULL;

    for (cur_node = node; cur_node != NULL; cur_node = cur_node->next)
    {
        // ENTER NODE
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            if (g_str_equal (cur_node->name, "b"))
                g_string_append (string, "<b>");
            else if (g_str_equal (cur_node->name, "i"))
                g_string_append (string, "<i>");
            else if (g_str_equal (cur_node->name, "u"))
                g_string_append (string, "<u>");
            else if (g_str_equal (cur_node->name, "br"))
                g_string_append (string, "\n");
            else if (g_str_equal (cur_node->name, "p"))
                g_string_append (string, "");
            else
                g_info ("Ignored element: %s\n", cur_node->name);
        }
        else if (cur_node->type == XML_TEXT_NODE)
        {
            gchar *escaped_text = g_markup_escape_text ((const gchar *)cur_node->content, -1);
            g_string_append (string, escaped_text);
            g_free (escaped_text);
        }

        // PROCESS CHILDREN
        build_text_recursive (cur_node->children, string);

        // EXIT NODE
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            if (g_str_equal (cur_node->name, "b"))
                g_string_append (string, "</b>");
            else if (g_str_equal (cur_node->name, "i"))
                g_string_append (string, "</i>");
            else if (g_str_equal (cur_node->name, "u"))
                g_string_append (string, "</u>");
        }
    }

    return string;
}

gchar *
exm_utils_convert_html (const gchar *html)
{
    htmlDocPtr doc;
    xmlNode *root;
    GString *string = g_string_new ("");

    doc = htmlParseDoc ((const xmlChar *)html, "UTF-8");
    if (doc == NULL)
    {
        g_critical ("Failed to parse HTML document.");
        return NULL;
    }

    root = xmlDocGetRootElement (doc);
    if (root == NULL)
    {
        g_warning ("Empty HTML document.");
        xmlFreeDoc (doc);
        xmlCleanupParser ();
        return NULL;
    }

    build_text_recursive (root, string);

    xmlFreeDoc (doc);
    xmlCleanupParser ();

    return g_string_free (string, FALSE);
}
