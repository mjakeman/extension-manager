/*
 * exm-types.h
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

#pragma once

// Should really be in the `local/` subdirectory but meson's mkenums
// has trouble with relative paths...

typedef enum
{
    EXM_EXTENSION_TYPE_SYSTEM = 1,
    EXM_EXTENSION_TYPE_PER_USER = 2,
} ExmExtensionType;

typedef enum
{
    EXM_EXTENSION_STATE_ACTIVE = 1,
    EXM_EXTENSION_STATE_INACTIVE = 2,
    EXM_EXTENSION_STATE_ERROR = 3,
    EXM_EXTENSION_STATE_OUT_OF_DATE = 4,
    EXM_EXTENSION_STATE_DOWNLOADING = 5,
    EXM_EXTENSION_STATE_INITIALIZED = 6,
    EXM_EXTENSION_STATE_DEACTIVATING = 7,
    EXM_EXTENSION_STATE_ACTIVATING = 8,
    EXM_EXTENSION_STATE_UNINSTALLED = 99,
} ExmExtensionState;

typedef enum
{
    EXM_INSTALL_BUTTON_STATE_DEFAULT,
    EXM_INSTALL_BUTTON_STATE_INSTALLING,
    EXM_INSTALL_BUTTON_STATE_INSTALLED,
    EXM_INSTALL_BUTTON_STATE_UNSUPPORTED
} ExmInstallButtonState;
