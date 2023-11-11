# Translating Extension Manager

## How to add new translation

1. You need software to translate Extension Manager. In this example, we will use POEdit: https://poedit.net/
2. Clone Extension Manager repository to your device. You can do this using command line. If you have git:
```bash
git clone git@github.com:mjakeman/extension-manager.git
```
Or you can click green button "Code", select "Download ZIP", which will download repository inside of zip file.

Unpack it with archive manager.

3. You need pot file, that lies inside `./po` folder. Make sure, that [pot file is up-to-date](## Regenerate POT files), before proceeding.
4. Open it with POEditor.
5. Click on button "Create new translation", select your desired language.
6. Press Ctrl+S to save new file, place it inside `./po` with filename that POEdit give you.
7. Open `./po/LINGUAS` file with any text editor, and add there locale code of you language. For example, if you want to add Ukrainian translation, POEdit will suggest you filename `uk.po`, so in `LINGUAS` file, you need to add `uk`.

But sometimes you might want to specify variant for your language. For example, for Russian in Russia Federation, you need to set locale to `ru_RU`, instead of just `ru`.

More about locales you can learn here: https://www.gnu.org/software/gettext/manual/html_node/Locale-Names.html

8. Translate Extension Manager!
9. [Test your translation!](## How to test translation)

## How to test translation

After you finished translation, you might want to test it. The easiest way to do so, is to use GNOME Builder.

1. If you doesn't have GNOME Builder, install it: [GNOME Builder](apps.gnome.org/Builder)
2. Open it.
3. Click on button below "Select a Folder..." and pick folder that with Extension Manager sources [that we cloned](## How to add new translation)
4. In upper-center, click on hammer button to build Extension Manager.
5. Then click on button with triangle pointing down.
6. In opened menu pick "Install".
7. Then open New Runtime Terminal via Ctrl+Alt+T or click plus button in top-left corner and pick "New Runtime Terminal"
8. In opened terminal, you need to force locale that you want to show. Usually, it will match locale name of your po file or it might require to specify your regional code. For example, to force application in Ukrainian language, you need to type:
```bash
LC_ALL=uk_UA.UTF-8
```
Where you need to replace `uk_UA` with your desired locale.

9. Run `extension-manager` to run Extension Manager with locale that you want to test.

Once you done some changes, you need to refresh application so it will apply changes to translation. To do so:

1. Stop current running Extension Manager with Ctrl+C in terminal or simply close Extension Manager window.
2. Then click on button with triangle pointing down in GNOME Builder.
3. In opened menu pick "Install".

After that, you can again run `extension-manager` in terminal, where your changes to po file will be applied.

## How to update existing translation
If you want update to update translation that someone else already did:

1. Try to contact original translator. You might find their contact info in top of po file or in `translator-credits` and ask them about changes in translation.
2. If you can't contact them, then just translate on your own.
3. If you wish, don't forget to add yourself to `translator-credits`.

Try to be consistent with choices that previous translator did, unless they contradict other GTK/GNOME applications translated terms.

## How to contribute your translation
1. Make sure, that you have account on Github. And if not, create one here: https://github.com/

2. Also make sure, that you know some basics about how to make pull requests: https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-a-pull-request-from-a-fork; And how to create fork: https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/working-with-forks/about-forks

3. Upload changes that you make in your local copy of repository to your fork.

If you already know how to use git:
```bash
cd extension-manager
git commit -m "I translated Extension Manager!"
git push
```

If not:

1. Open your fork on Github, navigate to `./po` folder.
2. Click "Add file" -> "Upload files" and pick your translated .po file and LINGUAS.
3. Make sure that everything correct, there no typos in translation, mistakes, you didn't upload wrong files by accident, etc.
4. Open pull request to Extension Manager and pick your fork: https://github.com/mjakeman/extension-manager/compare

## Tips about translating
1. Try to be consistent in your translation with other GTK/GNOME applications.

Pick same terms, use same accelerations, etc.

2. Always check your translation before opening pull request, don't just blindly translate.

3. Open your po file with any text efitor, not POEdit, and in top most file you will find some placeholder info for your translation file. Please, fill it, if you can. Take as example `uk.po` file.

4. `translator-credits` is optional and you not comfortable with sharing your alias or email or link, then leave it as-is. Also, if there will be credits for other translators, please, don't remove them. Just add your credit info on newline.

If you want to put email, do this:

`your_alias <somecooladress@mail.com>`

And if you want to place link:

`your_alias https://example.com`

You can check if you input everything correctly in "About" dialog of Extension Manager.

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
