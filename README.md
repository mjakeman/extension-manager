# Extension Manager
A native tool for browsing, installing, and managing GNOME Shell Extensions.

Written with GTK 4 and libadwaita.

![Screenshot of the main GUI](data/screenshot-combined.png)

## Features
The tool supports:
 - Browsing and searching extensions from `extensions.gnome.org`
 - Installation
 - Enabling and Disabling
 - Uninstall
 
Things that are not yet supported:
 - Updating extensions in-app
 - Translations (help wanted)

If there's something you'd like to see, contributions are welcome!

## Building
The easiest way to build is by cloning this repo with GNOME Builder. It
will automatically resolve all relevant flatpak SDKs automatically.

If you run into issues, make sure you have the [`gnome-nightly`](https://wiki.gnome.org/Apps/Nightly)
flatpak repository installed.
