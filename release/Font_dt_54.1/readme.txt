AmigaOS 4 Font DataType (v54.1)
===============================

OVERVIEW
--------
The font.datatype is a high-performance AmigaOS 4 component designed to provide 
system-wide visualization and metadata access for font files. It serves as a 
bridge between the AmigaOS DataType system and various font rendering engines, 
allowing applications like Multiview to preview fonts seamlessly.

GitHub: https://github.com/billborsari/font.datatype

KEY FEATURES
------------
- Minimalist Triage Logging: Consolidates debug output into T:font.datatype.log for easy troubleshooting without cluttering the system log.
- Dual-Engine Rendering: Supports both standard diskfont.library bitmap 
  rendering and high-fidelity ft2.library (FreeType) outline rendering.
- Modern Format Support: Native recognition and rendering of .font (Amiga 
  bitmap), .ttf (TrueType), .otf (OpenType), .pfb (PostScript Type 1), and 
  .otag (Amiga Outline) files.
- Dynamic Previews: Generates multi-size previews (11, 15, 20, 36, 50 pts) 
  with an on-screen indicator of the rendering engine used.
- System Localization: Automatically selects language-specific pangrams based 
  on the system locale (English, French, German, Spanish, and Polish supported).
- Robustness: Implements strict object lifecycle management and persistent 
  instance data to ensure system stability.

CHANGES SINCE 2008 VERSION
--------------------------
The 2026 update (v54.1) represents a significant refactoring of the legacy 
codebase to improve stability and modernize font handling:

1. Stability & Lifecycle Fixes
- Library Node Management: Resolved a critical "crash on second use" bug by 
  removing redundant AddLibrary() calls, allowing the OS ELF loader to manage 
  the library lifecycle correctly.
- Persistent Instance Data: Refactored the InstanceData structure to include 
  persistent storage for BitMapHeader and color tables. This ensures that 
  pointers passed to the picture.datatype superclass remain valid throughout 
  the object's life.
- Graceful Error Handling: Implemented robust failure detection in OM_NEW. The 
  datatype now correctly disposes of partially created objects if a font cannot 
  be rendered, preventing memory leaks and crashes with invalid files.

2. Enhanced Font Loading
- Multi-Stage Resolution: Implemented a three-tier loading strategy (Stripped 
  Name -> Full Path -> Forced FreeType Engine) to resolve path-related failures 
  common in legacy versions, preventing unexpected fallbacks to Topaz.
- Raw File Support: Added explicit support for fonts without .font descriptors 
  using the OT_FontFile and OT_FontFormat tags, allowing direct loading of raw 
  .ttf.
- Binary Signature Recognition: Updated the external Recognizer tool to identify 
  modern font formats (.ttf, .otf, .pfb, .otag) via binary magic numbers rather than 
  relying solely on file extensions.

3. Rendering Improvements
- FreeType Integration: Fully integrated ft2.library as an alternate rendering 
  path for high-quality outline previews.
- Topaz Fallback: Improved the fallback path to ensure that even unsupported or 
  malformed files provide a readable diagnostic preview instead of failing silently.
- Engine Diagnostics: The preview now includes a diagnostic header identifying 
  exactly which library and engine (Standard vs. FreeType) was used for rendering.

4. Localization
- Locale Integration: Integrated locale.library to detect the system language 
  and provide relevant localized pangrams, ensuring the preview is useful for 
  users in different regions.

INSTALLATION
------------
1. Copy Classes/DataTypes/font.datatype to your system's SYS:Classes/DataTypes/ drawer.
2. Copy Devs/DataTypes/Font to your system's SYS:Devs/DataTypes/ drawer.
3. Ensure ft2.library is installed in your system's LIBS: drawer for modern 
   outline font support.
4. Reboot your Amiga or restart the datatype system to load the new version.

CREDITS
-------
Based on the original Font DataType by Michael Letowski.
Updated for AmigaOS 4.1+ by Bill Borsari and Antigravity (2026).
Source available at: https://github.com/billborsari/font.datatype
