# Falcom BGM Info

## Introduction
**Falcom BGM Info** is a quality-of-life mod for various Nihon Falcom games on PC. It displays an unobtrusive "Toast" notification at the top of the screen whenever the background music changes, showing the **Song Title**, **Japanese Title**, **Album**, and **Disc/Track number**, similar to how the newer PH3 Ports (Ys X: Nordics, and the Calvard Arc Trails games) do.

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
    * This should include `winmm.dll`, and the `assets/` folder.
    * **For Xanadu Next users:** Ensure `d3d8.dll` is also extracted.
4.  Launch the game. The mod will automatically detect the game and load the correct music map.

### Steam Deck / Linux (Proton)

1.  Follow the **Windows** installation steps above (copy files to the game folder).
2.  Rename the winmm.dll to one of the following names:
- xinput1_4.dll: For Trails in the Sky the 1st.
- dinput8.dll: For most games (needs testing yet).
- winmm.dll: For some games (needs testing yet).
- version.dll: For a few games (needs testing yet).
3.  In your Steam Library, right-click the game and select **Properties**.
4.  In the **General** tab, find **Launch Options**.
5.  Add the following command to force the mod to load: (Remember to put your .dll name!)

    **For most games:**
    ```bash
    WINEDLLOVERRIDES="yourdllnamehere=n,b" %command%
    ```

    **For Xanadu Next ONLY:**
    ```bash
    WINEDLLOVERRIDES="yourdllnamehere,d3d8=n,b" %command%
    ```

---

## Troubleshooting
* **Crash on Linux?** Ensure you are using the correct Launch Option string above. If the game crashes immediately, verify you downloaded the correct version (Modern vs. Retro) matching the game's engine.
