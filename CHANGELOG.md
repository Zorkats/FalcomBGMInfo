# Changelog

Each release has a `## [vX.Y.Z] Title` section. The release workflow uses the title and the notes under it for the GitHub release, so keep that heading format.

## [v2.3.0] Update 2.3 - The SpecialK Update

### SpecialK compatibility
* Fixed a white screen and crash at startup when SpecialK is installed next to BGMInfo. BGMInfo's file hook made SpecialK treat its own controller checks as game calls, and they looped until the game crashed.
* The popup now handles swap chain resizes made through `ResizeBuffers1`, which SpecialK uses. Resolution, fullscreen and HDR changes no longer conflict with the popup.
* The same file hook fix is included in the Retro (DX9) build.

### Popups
* Fixed popups showing the wrong track, or not showing at all, when a game opened other sound files around a music change (reported in Ys IX). Only files listed in the game's music map can trigger a popup now, and the newest track always wins.
* File names now match regardless of letter case or slash direction.

### Music maps
* **Ys IX:** fixed the map layout so every track is recognized. The soundtrack has no Japanese titles, so the Japanese line is hidden. Fixed the track number of "New Life".
* **Ys VIII:** fixed the disc number of Dana's track and the spelling of "Sibylline", "Origine" and "Whimsical".
* **Ys: Memories of Celceta:** fixed a broken separator in one entry.
* **Ys: The Oath in Felghana:** corrected six Japanese titles against the official soundtrack, including the PC-88 and X68000 versions. Tracks that only have English titles now hide the Japanese line.

### Other
* Trails in the Sky the 1st 1.07 compatibility is now part of every build, instead of a patched Modern x64 binary.
* Release packages are now built by GitHub Actions.
