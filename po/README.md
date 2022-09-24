# Translating Extension Manager

## Translation Guide
Work in progress - please check back later

## Regenerate POT files
If you want to regenerate the POT file (i.e. when it hasn't been updated in
a while), there are a few steps that need to be followed:

### Update POTFILES with latest source files
Replace the contents of `POTFILES` with the output of the `print-source-files.sh`
script. Make sure to run it from this directory (`./po`).

### Build POT file
Go to the build directory (typically `_build`, but whichever you specified
when running meson).

Run the following command. This will recreate the POT file.

```
meson compile extension-manager-pot
```
