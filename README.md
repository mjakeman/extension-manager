# Extension Manager
A native tool for browsing, installing, and managing GNOME Shell Extensions.

Written with GTK 4 and libadwaita.

![Screenshot of the main GUI (light mode)](data/screenshot-combined.png#gh-light-mode-only)
![Screenshot of the main GUI (dark mode)](data/screenshot-combined-dark.png#gh-dark-mode-only)

## Features
The tool supports:
 - Browsing and searching extensions from `extensions.gnome.org`
 - Installation
 - Enabling and Disabling
 - Uninstall
 - Screenshots &amp; Images
 - Translations ([add your language!](https://github.com/mjakeman/extension-manager/issues/27))
 
Things that are not yet supported:
 - Updating extensions in-app
 - Ratings &amp; Comments

If there's something you'd like to see, contributions are welcome!

## Installing
Flatpak is the recommended way to install Extension Manager. 

You can get the latest version from flathub by clicking the button below. There
may also be independently-maintained packages available for your distribution.

<a href="https://flathub.org/apps/details/com.mattjakeman.ExtensionManager">
<img src="https://flathub.org/assets/badges/flathub-badge-i-en.png" width="190px" />
</a>

You can also install it through the AUR if you use Arch Linux:
[extension-manager](https://aur.archlinux.org/packages/extension-manager), [extension-manager-git](https://aur.archlinux.org/packages/extension-manager-git)

## Translations
Extension Manager has been translated into several different languages. Ideally, the
program will respect your system language out-of-the-box. However, you may need to take
some additional steps in order for flatpak to recognise your chosen locale. The
following workaround may work for you:

Set the languages you wish to use explicitly (e.g. `en` for English, `es` for Español):
```
# Optionally add --user if installed in a user prefix
flatpak config --set languages 'en;es'
```

Then update:
```
flatpak update
```

Now Extension Manager should respect your system language.

## Building
The easiest way to build is by cloning this repo with GNOME Builder. It
will automatically resolve all relevant flatpak SDKs automatically.

If you run into issues, make sure you have the [`gnome-nightly`](https://wiki.gnome.org/Apps/Nightly)
flatpak repository installed.
