/* extension.js
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

'use strict'

const { GLib, Gio, GObject, Shell, St } = imports.gi;
var oldMethod = null;

function fixUpdateMechanism() {
    // GNOME Shell currently will only check for updates if the official
    // org.gnome.Extensions app is installed. We modify this check so
    // that updating can take place if Extension Manager is the sole
    // manager app on the system.

    // Get the shell's built-in extension manager object
    var shellExtensionManager = imports.ui.extensionSystem.ExtensionManager;

    // Save the old method to restore later
    oldMethod = Object.getOwnPropertyDescriptor(shellExtensionManager.prototype, 'updatesSupported').get.bind({});

    // Add workaround for Extension Manager
    Object.defineProperty(shellExtensionManager.prototype, 'updatesSupported', {
        get: function() {
            const appSys = Shell.AppSystem.get_default();
            return (appSys.lookup_app('org.gnome.Extensions.desktop') != null) ||
                   (appSys.lookup_app('com.mattjakeman.ExtensionManager.desktop') != null);
        }
    });
}

function unfixUpdateMechanism() {
    // Get the shell's built-in extension manager object
    var shellExtensionManager = imports.ui.extensionSystem.ExtensionManager;

    // Restore old method
    Object.defineProperty(shellExtensionManager.prototype, 'updatesSupported', {
        get: oldMethod
    });
}

class Extension {
    constructor() {
    }

    enable() {
        fixUpdateMechanism();
    }

    disable() {
        unfixUpdateMechanism();
    }
}

function init() {
    return new Extension();
}
