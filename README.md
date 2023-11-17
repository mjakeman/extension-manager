<div align="center">
  <img src="/data/icons/com.mattjakeman.ExtensionManager.svg" width="64">
  <h1 align="center">Extension Manager</h1>
  <p align="center">A native tool for browsing, installing, and managing GNOME Shell Extensions</p>
  
  [![Build Status](https://img.shields.io/github/actions/workflow/status/mjakeman/extension-manager/main.yml?branch=master)](https://github.com/mjakeman/extension-manager/actions/workflows/main.yml)
[![Release Version](https://img.shields.io/github/v/release/mjakeman/extension-manager)](github.com/mjakeman/extension-manager/releases/latest)
[![Downloads](https://img.shields.io/badge/dynamic/json?color=green&label=downloads&query=installs_total&url=https%3A%2F%2Fflathub.org%2Fapi%2Fv2%2Fstats%2Fcom.mattjakeman.ExtensionManager)](https://flathub.org/apps/details/com.mattjakeman.ExtensionManager)
[![License (GPL-3.0)](https://img.shields.io/github/license/mjakeman/extension-manager)](http://www.gnu.org/licenses/gpl-3.0)

  <sup>Written with GTK 4 and libadwaita</sup>
  
![Screenshot of the main GUI (light mode)](data/screenshot-combined.png#gh-light-mode-only)
![Screenshot of the main GUI (dark mode)](data/screenshot-combined-dark.png#gh-dark-mode-only)

</div>

## 📋 Features
The tool supports:
 - Browsing and searching extensions from `extensions.gnome.org`
 - Installation and Removal
 - Enabling and Disabling
 - Updating in-app (See 'Known Issues')
 - Screenshots &amp; Images
 - Ratings &amp; Comments
 - Translations ([add your language!](#-translations))

If there's something you'd like to see, contributions are welcome!

## ⚠️ Known Issues
### Extensions are not being updated
Updates do not work out of the box on GNOME 40 and certain older versions of GNOME
41 and 42 **unless the official GNOME Extensions app is also installed**. See here
for details and a simple workaround: [Wiki Page](https://github.com/mjakeman/extension-manager/wiki/Known-Issue:-Updates)

## 💬 Community
We now have a matrix room for Extension Manager.

Join and say hello! https://matrix.to/#/#extension-manager:matrix.org

## 💻 Installing
Flatpak is the recommended way to install Extension Manager. 

You can get the latest version from flathub by clicking the button below. There
may also be independently-maintained packages available for your distribution.

<a href="https://flathub.org/apps/details/com.mattjakeman.ExtensionManager">
<img src="https://flathub.org/assets/badges/flathub-badge-i-en.png" width="190px" />
</a>

### Third Party Packages
You may also be able to obtain Extension Manager from your distribution's package manager. Note these packages are maintained independently and thus may differ from the official version on Flathub. Please report any issues experienced to the package maintainer.

[![Packaging status](https://repology.org/badge/vertical-allrepos/extension-manager.svg)](https://repology.org/project/extension-manager/versions)

## 🌐 Translations
Contributions to translations are always welcome!

If you are totally new to GTK software translation (or software development in general), fear not! The entire process is explained in as much detail as possible [here](/po/README.md)!

If you have any trouble, please [get in touch](#-community).

## ⏰ Using Unsupported Extensions
GNOME Shell will not load extensions that were not designed for your current
version. You can override this behaviour by manually disabling GNOME Shell's
version check. Extension Manager will respect this preference and allow you
to use unsupported extensions fully.

Note that unsupported extensions will likely not work as intended and
may introduce instability to your system. The version check should therefore
be disabled at your own risk.

Turn off the version check and allow unsupported extensions:

```
gsettings set org.gnome.shell disable-extension-version-validation true
```

Use the default setting and return to safety:
```
gsettings reset org.gnome.shell disable-extension-version-validation
```

## 🔨 Building
The easiest way to build is by cloning this repo with GNOME Builder. It
will automatically resolve all relevant flatpak SDKs automatically.

Extension Manager needs the GNOME 43 SDK in order to build.

### Dependencies
Extension Manager depends on the following libraries:
 - gettext
 - gtk4
 - libadwaita
 - libjson-glib
 - libsoup
 - [blueprint](https://gitlab.gnome.org/jwestman/blueprint-compiler)
 - [text-engine](https://github.com/mjakeman/text-engine/)

On Debian-based distributions, the required dependencies can be installed with the following command:
```shell
sudo apt install blueprint-compiler gettext libadwaita-1-dev libgtk-4-dev libjson-glib-dev libsoup-3.0-dev libtext-engine-dev meson
```

### Building From Source
```shell
meson setup _build
ninja -C _build
ninja install -C _build
```
