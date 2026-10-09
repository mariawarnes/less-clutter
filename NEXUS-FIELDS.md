# Copy-ready Nexus fields

## Page fields

| Field | Value |
| --- | --- |
| Game | Skyrim Special Edition |
| Mod name | Less Clutter |
| Version | 1.2.0 |
| Author | mlw05 |
| Category | Gameplay (or Utilities, whichever Nexus offers) |
| Adult content | No |
| Language | English |
| Tags | AI-Generated Content; Skyrim Script Extender (SKSE); Utilities for Players if offered |

Add no VR, performance or compatibility tags that have not been established. The AI Media tag is not needed: the mod has no AI-generated images, audio or video.

## Short description

Removes or locks in place the useless clutter in Skyrim (bowls, plates, tankards, linen wraps and so on), plus empty containers. Configurable item list. Requires SKSE64 and Address Library for Skyrim 1.7.104.

## Requirements

- Address Library for SKSE Plugins: https://www.nexusmods.com/skyrimspecialedition/mods/32444 (the All in One file for 1.7.104.0).
- Skyrim Script Extender (SKSE64): https://skse.silverlock.org/ (2.3.1 for Steam Skyrim 1.7.104.0).
- No ESP/ESL, SkyUI, MCM or Creation Kit requirement.

## Main file

Filename: LessClutter-1.2.0.zip. Display name: Less Clutter 1.2.0. Install through Vortex as Default.

File description: Native SKSE plugin for Steam Skyrim 1.7.104.0 only. Requires SKSE64 and Address Library. Removes or locks useless items and empty containers when cells load. Never touches player-dropped or script-spawned items, quest-aliased items, or containers in player homes. Edit LessClutter.ini to change the item list. No ESP.

## Miscellaneous file

Filename: LessClutter-1.2.0-Source.zip. Display name: Less Clutter 1.2.0 - Corresponding Source. GPL-3.0-or-later with included exceptions. For developers and licence compliance; not required to play.

## Permissions and credits

Third-party content question: Yes, used under the included licences. Do not select "all assets are mine".

Credits text:

CommonLibSSE-NG / alandtse CommonLibVR and its contributors; spdlog and fmt contributors; the SKSE64 team; meh321 for Address Library; the Skyrim Wiki "Useless Items" list (CC BY-NC-SA) for the default item names. All required notices are included. Code and documentation were developed using Claude (Anthropic). No Bethesda assets are redistributed.

Custom permissions text:

Less Clutter's code is GPL-3.0-or-later with the Modding Exception and GPL-3.0 Linking Exception supplied in the downloads. Use, modification and redistribution are permitted under those terms, including their source and notice obligations. Third-party material retains its original licences. Please read LICENSING.md, COPYING and EXCEPTIONS.md. Source: https://github.com/mariawarnes/less-clutter

Choose the reuse, upload, modification and conversion options that are consistent with the GPL for the code, rather than requiring extra permission. Donation and reward-program participation is your choice. No console conversion is supplied.

## Changelog for 1.2.0

- Containers holding only useless items now count as empty.
- Fixed items not being removed in areas entered after loading (dungeons): now acts on cell attach and cell load.
- Empty containers option (EmptyContainers=Off/Remove/Fix) skips player homes and quest-aliased containers.
- Added Debug option that logs what the crosshair target is and what the mod decides.
- Renamed from Useless Items to Less Clutter.

Known limitation: a quest that refers to a container without an alias is not detectable; use EmptyContainers=Fix if you hit one.
