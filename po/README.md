<!--
SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
SPDX-License-Identifier: GPL-2.0-or-later
-->

# Translations

Sirkel has two translation templates:

- `sirkel.pot` - the shortcut names and direction labels (used by the effect and the settings)
- `kwin_effect_sirkel_config.pot` - the settings window

## Adding a language

Create a folder named after the language code and start from both templates, for example for German:

```sh
mkdir po/de
msginit --locale=de --input=po/sirkel.pot --output=po/de/sirkel.po
msginit --locale=de --input=po/kwin_effect_sirkel_config.pot --output=po/de/kwin_effect_sirkel_config.po
```

Translate the `.po` files with any editor (Lokalize, Poedit, or a text editor), then build and install as usual. The translations are compiled and installed with the rest of Sirkel.

## After changing strings in the code

Run `scripts/update-translations.sh`. It regenerates both templates and merges new strings into every existing translation.
