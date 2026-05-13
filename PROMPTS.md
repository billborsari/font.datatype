# Project Prompt Log

This log captures all the prompts provided during the development of the `font_dt` project.

| Date | Prompt | Summary of Action |
|------|--------|-------------------|
| 2026-05-04 | build the font_dt project to see if it will build in it's current state | Converted source to UTF-8, fixed OS4 API compatibility in `prefs.c`, `dispatch.c`, and `otag.c`, added `newlib` support, and successfully built `font.datatype`. |
| 2026-05-04 | I would like to create a special log that captures all the prompts I provide working on this project, is that possible? | Created `PROMPTS.md` to track project history. |
| 2026-05-04 | yes, summarize action taken | Updated `PROMPTS.md` to include a "Summary of Action" column. |
| 2026-05-04 | bring this code up to the latest library and API calls, like allocvec for memory, etc | Modernized memory allocation to `AllocVecTags`, updated API calls to use modern interfaces, and resolved macro expansion conflicts. |
| 2026-05-04 | update the make file to ensure when there is a crash the module line number is included | Updated `Makefile.gcc` with `-Og`, `-ggdb3`, and `-fno-omit-frame-pointer` for high-fidelity crash reporting. |
| 2026-05-05 | various prompts regarding font detection and logging | Expanded font detection to support TTF, OTF, WOFF/2, and collections. Updated DataType descriptors to recognize these extensions. Implemented robust per-line timestamped logging using native AmigaOS Utility calls. |

