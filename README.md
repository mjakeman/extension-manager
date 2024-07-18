<div align="center">
  <img src="/data/icons/com.mattjakeman.ExtensionManager.svg" width="64">
  <h1 align="center">Extension Manager</h1>
  <p align="center">A native tool for browsing, installing, and managing GNOME Shell Extensions</p>
  
  [![Build Status](https://img.shields.io/github/actions/workflow/status/mjakeman/extension-manager/main.yml?branch=master)](https://github.com/mjakeman/extension-manager/actions/workflows/main.yml)
[![Translation status](https://hosted.weblate.org/widget/extension-manager/svg-badge.svg)](https://hosted.weblate.org/engage/extension-manager/)
[![Release Version](https://img.shields.io/github/v/release/mjakeman/extension-manager)](github.com/mjakeman/extension-manager/releases/latest)
[![Downloads](https://img.shields.io/badge/dynamic/json?color=green&label=downloads&query=installs_total&url=https%3A%2F%2Fflathub.org%2Fapi%2Fv2%2Fstats%2Fcom.mattjakeman.ExtensionManager)](https://flathub.org/apps/details/com.mattjakeman.ExtensionManager)
[![License (GPL-3.0)](https://img.shields.io/github/license/mjakeman/extension-manager)](http://www.gnu.org/licenses/gpl-3.0)

  <sup>Written with GTK 4 and libadwaita</sup>
  
![Screenshot of the main GUI (light mode)](data/screenshot-installed.png#gh-light-mode-only)
![Screenshot of the main GUI (dark mode)](data/screenshot-browse-dark.png#gh-dark-mode-only)

</div>

## 📋 Features
The tool supports:
 - Browsing and searching extensions from `extensions.gnome.org`
 - Installation and Removal
 - Enabling and Disabling
 - Updating in-app ([GNOME 43+](https://github.com/mjakeman/extension-manager/wiki/Known-Issue:-Updates))
 - Screenshots &amp; Images
 - Ratings &amp; Comments
 - Translations ([add your language!](#-translations))

If there's something you'd like to see, contributions are welcome!

## 💬 Community
We now have a matrix room for Extension Manager.

Join and say hello! https://matrix.to/#/#extension-manager:matrix.org

## 💻 Installing
Flatpak is the recommended way to install Extension Manager. 

You can get the latest version from flathub by clicking the button below. There
may also be independently-maintained packages available for your distribution.

<a href='https://flathub.org/apps/com.mattjakeman.ExtensionManager'>
<img width='240' alt='Get it on Flathub' src='https://flathub.org/api/badge?locale=en'/>
</a>

### Third Party Packages
You may also be able to obtain Extension Manager from your distribution's package manager.

> [!IMPORTANT]
> These packages are **maintained independently** and thus may differ from the official version on Flathub. There is no guarantee of support. Please report any issues experienced to the package maintainer (not here!).

[![Packaging status](https://repology.org/badge/vertical-allrepos/extension-manager.svg)](https://repology.org/project/extension-manager/versions)

## 🌐 Translations
Extension Manager is translated into more than 30 languages.

> [!NOTE]
> We use [Weblate](https://weblate.org/en/) - an open source continuous localisation tool - for translation management. Access to Hosted Weblate is kindly provided free of charge to the Extension Manager project.

### Contribute
Contributions to translations are always welcome!

We have a comprehensive translation guide [here](/po/README.md).

If you are new to Localisation (l10n), fear not! The entire process is explained above in as much detail as possible. If you have any trouble, please also [get in touch](#-community).

## ⚠️ Known Issues
### Extensions are not being updated
Updates do not work out of the box on GNOME 40 and certain older versions of GNOME
41 and 42 **unless the official GNOME Extensions app is also installed**. See here
for details and a simple workaround: [Wiki Page](https://github.com/mjakeman/extension-manager/wiki/Known-Issue:-Updates)

## ⏰ Using Unsupported Extensions
GNOME Shell will not load extensions that were not designed for your current
version. You can override this behaviour by manually disabling GNOME Shell's
version check. Extension Manager will respect this preference and allow you
to use unsupported extensions fully.

> [!CAUTION]
> Unsupported extensions will likely not work as intended and
> may introduce instability to your system. Disable the version check at your own risk.

> [!IMPORTANT]
> Re-enable the version check before filing issues against GNOME components.

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

Extension Manager needs a recent version of the GNOME SDK in order to build. See the [Development](build-aux/com.mattjakeman.ExtensionManager.Devel.json) or [Stable](/build-aux/com.mattjakeman.ExtensionManager.json) Flatpak manifests for a full dependency list.

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
