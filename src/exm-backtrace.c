/*
 * exm-backtrace.c
 *
 * Copyright 2022-2025 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#include "exm-config.h"

#include "exm-backtrace.h"

#include <glib.h>
#include <stdint.h>

#if WITH_BACKTRACE
#include <backtrace-supported.h>
#include <backtrace.h>
#endif

#if WITH_BACKTRACE
static struct backtrace_state *state = NULL;
static int frames_omitted_count = 0;

static void
exm_backtrace_error_cb (void       *data G_GNUC_UNUSED,
                        const char *msg,
                        int         errnum)
{
    g_warning ("Error (%d): %s\n", errnum, msg);
}

static int
exm_backtrace_full_cb (void       *data,
                       uintptr_t   pc G_GNUC_UNUSED,
                       const char *filename,
                       int         lineno,
                       const char *function)
{
    GString *string_builder = (GString *) data;
    char *entry;

    entry = g_strdup_printf ("%s:%d '%s'\n", filename, lineno, function);

    if (strstr (entry, "null") != NULL)
        frames_omitted_count++;
    else
    {
        if (frames_omitted_count > 0)
        {
            g_string_append_printf (string_builder, "%d frame%s omitted\n",
                                    frames_omitted_count,
                                    (frames_omitted_count > 1) ? "s" : "");
            frames_omitted_count = 0;
        }
        g_string_append (string_builder, entry);
        g_free (entry);
    }

    return 0;
}
#endif

void
exm_backtrace_init (char *filename G_GNUC_UNUSED)
{
#if WITH_BACKTRACE

# ifdef BACKTRACE_SUPPORTED
    state = backtrace_create_state (filename, 0,
                                    exm_backtrace_error_cb,
                                    NULL);
# else
    g_warning ("Backtraces are not supported.\n");
# endif

#else
    g_info ("Backtraces were not enabled at build time.\n");
#endif
}

char *
exm_backtrace_print ()
{
#if WITH_BACKTRACE
    GString *string_builder;

    if (!state)
    {
        g_critical ("Cannot print backtrace.\n");
        return NULL;
    }

    string_builder = g_string_new ("");

    backtrace_full (state, 0,
                    exm_backtrace_full_cb,
                    exm_backtrace_error_cb,
                    string_builder);

    return g_string_free (string_builder, FALSE);
#else
    g_critical ("Backtraces were not enabled at build time.\n");
    return NULL;
#endif
}
