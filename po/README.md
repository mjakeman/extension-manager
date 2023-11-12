# Translating Extension Manager

## How to add new translation

1. You need software to translate Extension Manager (in this case, we will use [POEdit](https://poedit.net/)) and [GNOME Builder](https://apps.gnome.org/Builder/) to build and test your translation.
2. Clone Extension Manager repository to your device.

* Using git:
```bash
git clone git@github.com:mjakeman/extension-manager.git
```

* Using Github:

1. Click green button `Code`.
2. In opened menu click `Download ZIP`.
3. Save it in your `~/` home folder.
4. Unpack archive with archive manager of your choice.
5. Rename folder that will be unpacked from archive to `extension-manager`.

* Using GNOME Builder:

1. Run GNOME Builder.
2. Click `Clone Repository` button on bottom.
3. In `Repository URL` paste: `https://github.com/mjakeman/extension-manager`
4. Set `Location` to `~/`

Since later we will use GNOME Builder again, you can leave it opened.


3. You need `pot` file, that lies inside `./po` folder. Make sure, that [pot file is up-to-date](#regenerate-pot-files), before proceeding.
4. Open it with POEditor.
5. Click on button on bottom of window `Create a new translation`, select your desired language.
6. Press `Ctrl+S` to save new file, place it inside `./po` folder with filename that POEdit gave you.
7. Open `./po/LINGUAS` file with any text editor, and add there locale code of your language. For example, if you want to add Ukrainian translation, POEdit will suggest you filename `uk.po`, so in `LINGUAS` file, you need to add `uk`. And, please, keep locale codes in alphabetical order.

But sometimes, you might want to specify variant for your language. For example, for Russian in Russia Federation, you need to set locale to `ru_RU`, instead of just `ru`.

More about locales you can learn [here](https://www.gnu.org/software/gettext/manual/html_node/Locale-Names.html).

8. Translate Extension Manager!
9. [Test your translation](#how-to-test-translation).

## How to test translation

After you finished translation, you might want to test it. The easiest way to do so, is to use [GNOME Builder](https://apps.gnome.org/Builder/).

If you choose to clone repository using GNOME Builder in [How to add new translation](#how-to-add-new-translation) and didn't closed GNOME Builder window, skip to step 4.

1. If you didn't already installed [GNOME Builder](https://apps.gnome.org/Builder/), install it now.
2. Open GNOME Builder.
3. Click on button on bottom of window `Select a Folder...` and pick folder with Extension Manager source [that we cloned before](#how-to-add-new-translation).
4. In top-center, click on button with `triangle pointing down`.
5. In opened menu pick `Rebuild`.
6. Wait until you get `Build succeeded` in upper-center text box.
7. Click on button with `triangle pointing down` in upper-center.
8. In opened menu pick `Install`.
9. Open `New Runtime Terminal`.

With `Ctrl+Alt+T` shortcut or click `+` button in top-left corner and pick `New Runtime Terminal`.

10. In opened terminal, you need to force locale that you want to test. Usually, it will match locale name of your `po` file or it might require to specify your regional code. For example, to force application in Ukrainian language, you need to type:
```bash
LC_ALL=uk_UA.UTF-8
```
Where you need to replace `uk_UA` with your desired locale.

More about locales you can learn [here](https://www.gnu.org/software/gettext/manual/html_node/Locale-Names.html).

11. Run `extension-manager` to run Extension Manager, which would use locale that you want to test.

Once you done some changes, you need to refresh application so it will apply changes to translation. To do so:

1. Stop current running Extension Manager with `Ctrl+C` in terminal or simply close Extension Manager window.
2. Then click on button on `top-center` with `triangle pointing down` in GNOME Builder.
3. In opened menu pick `Rebuild`.
4. Wait until GNOME Builder rebuild Extension Manager.
5. Now close this terminal with:
```bash
exit
```
Or just click `cross symbol` on tab with this terminal.

6. After that, repeat instructions from [How to test translation](#how-to-test-translation) section, starting from step 7.
Do this every time, when you do some changes to `po` file to see them in Extension Manager.

## How to update existing translation
If you want update to update translation that someone else already did:

1. Try to contact original translator. You might find their contact info in top of `po` file of translation that you want to update (if you will open it via any text editor) or in `translator-credits` and ask them about changes in translation.
2. If you can't contact them for some reasons or they don't respond, then just translate on your own.
3. [Re-generate pot file](#regenerate-pot-files).
4. Open `po` file that you want to update with POEditor.
5. In top menu click `Translation`, then `Update from POT file…`, and pick `pot` file that you re-generated earlier.
6. If you wish, don't forget to add yourself to `translator-credits` and in top of `po` file via text editor.

Try to be consistent with choices that previous translator did, unless they contradict other GTK/GNOME applications translated terms or you think that your translation will be better.

## How to contribute your translation
1. Make sure, that you have account on [Github](https://github.com/). And if not, create one.

2. Also make sure, that you know some basics about how to make pull requests: [How to make pull request from fork](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-a-pull-request-from-a-fork) and [How to create fork](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/working-with-forks/about-forks).

3. Upload changes that you make in your local copy of repository to your Github fork.

If you already know how to use git, this can be done like this:
```bash
cd extension-manager
git commit -m "I translated Extension Manager!"
git push
```

If not, you could use Github interface to do so:

1. Open your fork on Github, navigate to `./po` folder.
2. Click `Add file` then `Upload files` and pick your translated `po` file and `LINGUAS`.
3. Make sure that everything correct, there no typos in translation, mistakes, you didn't upload wrong files by accident, etc.
4. Open [pull request](https://github.com/mjakeman/extension-manager/compare) to Extension Manager repository against your fork.

## Tips about translating
1. Try to be consistent in your translation with other GTK/GNOME applications.

Pick same terms, use same accelerators, etc.

2. Always test your translation before opening pull request.

3. Open your `po` file with any text editor, not with POEdit, and in top most of file you will find some placeholder info for your translation. Please, fill it, if you can. Take as example `uk.po` file.

4. `translator-credits` is optional and you not comfortable with sharing your alias or email or link, then leave it as-is. Also, if there will be credits for other translators, please, don't remove them. Just add your credit info on newline.

If you want to put email, do this:

`your_alias <somecooladress@mail.com>`

And if you want to place link:

`your_alias https://example.com`

You can check if you input everything correctly in `About` dialog of Extension Manager.

## Regenerate POT files
If you want to regenerate the `pot` file (i.e. when it hasn't been updated in
a while), there are a few steps that need to be followed:

GNOME Builder version:

1. Open GNOME builder.
2. Open `New Runtime Terminal` with `Ctrl+Alt+T` or click `plus button` in top-left corner and pick `New Runtime Terminal`
3. And replace contents of `POTFILES` with output of `print-source-files.sh`.

```bash
cd extension-manager/po
./print-source-files.sh > ./POTFILES
```
4. Go to the build directory (typically `_build`, but whichever you specified when running meson) and initialize it.

Like this:

```bash
cd
cd extension-manager
mkdir ./_build
cd ./_build
meson
```

6. Now meson is initialized and you can re-generate `pot` file.

Like this:
```bash
meson compile extension-manager-pot
```
7. Now close this terminal with:
```bash
exit
```
Or just click `cross symbol` on tab with this terminal.

Without GNOME Builder:

1. Make sure that you have `meson` and `gettext` utilities installed on your system. Refer to your distribution package manager.
2. Go to `po` directory and run `print-source-files.sh`.

```bash
cd extension-manager/po
./print-source-files.sh > ./POTFILES
```
3. Go to the build directory (typically `_build`, but whichever you specified when running meson) and initialize it.

Like this:

```bash
cd extension-manager
mkdir ./_build
cd ./_build
meson
```

6. Now meson is initialized and you can re-generate `pot` file.

Like this:
```bash
meson compile extension-manager-pot
```