# Falcom BGM Info Mod

## Introduction
**Falcom BGM Info** is a quality-of-life mod for various Nihon Falcom games on PC. It displays an unobtrusive "Toast" notification at the top of the screen whenever the background music changes, showing the **Song Title**, **Japanese Title**, **Album**, and **Disc/Track number**.

It is designed to be lightweight, compatible with Steam Deck/Linux, and requires no configuration—just drag and drop to install.

---

## Compatible Games

### Modern (DirectX 11)
* **The Legend of Heroes: Trails in the Sky the 1st** (Remake)
* **The Legend of Heroes: Trails of Cold Steel I & II**
* **The Legend of Heroes: Trails of Cold Steel III & IV**
* **The Legend of Heroes: Trails into Reverie**
* **The Legend of Nayuta: Boundless Trails**
* **Ys: Memories of Celceta**
* **Ys VIII: Lacrimosa of DANA**
* **Ys IX: Monstrum Nox**

### Retro (DirectX 9)
* **Xanadu Next** (Requires Wrapper)
* **Ys SEVEN**
* **Ys Origin**
* **Ys: The Oath in Felghana**
* **Ys VI: The Ark of Napishtim**
* **The Legend of Heroes: Trails in the Sky FC** (Original DX9 Version)

---

## Installation Instructions

### Windows

1.  Download the release `.zip` matching your game engine:
    * **Modern_DX11.zip**: For Cold Steel, Reverie, Ys 8/9, Nayuta, Sky Remake.
    * **Retro_DX9.zip**: For Xanadu Next, Ys Origin, Ys 6/7, Sky FC (Original).
2.  Open your game's installation folder (where the game's `.exe` is located).
3.  **Extract all files** from the zip into the game directory.
    * This should include `winmm.dll`, the `.asi` file, and the `assets/` folder.
    * **For Xanadu Next users:** Ensure `d3d8.dll` is also extracted.
4.  Launch the game. The mod will automatically detect the game and load the correct music map.

### Steam Deck / Linux (Proton)

1.  Follow the **Windows** installation steps above (copy files to the game folder).
2.  In your Steam Library, right-click the game and select **Properties**.
3.  In the **General** tab, find **Launch Options**.
4.  Add the following command to force the mod to load:

    **For most games:**
    ```bash
    WINEDLLOVERRIDES="winmm=n,b" %command%
    ```

    **For Xanadu Next ONLY:**
    ```bash
    WINEDLLOVERRIDES="winmm,d3d8=n,b" %command%
    ```

---

## Troubleshooting
* **Crash on Linux?** Ensure you are using the correct Launch Option string above. If the game crashes immediately, verify you downloaded the correct version (Modern vs. Retro) matching the game's engine.
