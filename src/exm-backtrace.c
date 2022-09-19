/* exm-backtrace.c
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

#include "exm-backtrace.h"

#include <glib.h>

#include <backtrace-supported.h>
#include <backtrace.h>

static struct backtrace_state *state = NULL;

static void
exm_backtrace_error_cb (void       *data,
                        const char *msg,
                        int         errnum)
{
    g_print ("Error (%d): %s\n", errnum, msg);
}

static int
exm_backtrace_full_cb (void       *data,
                       uintptr_t   pc,
                       const char *filename,
                       int         lineno,
                       const char *function)
{
    if (filename || lineno || function)
        g_print ("%s:%d '%s'\n", filename, lineno, function);

    return 0;
}

void
exm_backtrace_init (char *filename)
{
#ifdef BACKTRACE_SUPPORTED
    state = backtrace_create_state (filename, 0,
                                    exm_backtrace_error_cb,
                                    NULL);
#else
    g_warning ("Backtraces are not supported.\n");
#endif
}

void
exm_backtrace_print ()
{
    if (!state)
    {
        g_critical ("Cannot print backtrace.\n");
        return;
    }

    backtrace_full (state, 0,
                    exm_backtrace_full_cb,
                    exm_backtrace_error_cb,
                    NULL);
}
