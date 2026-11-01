# ClipSelector

A Qt 6 (C++/QML) video editing tool with two project modes:

- **MUSIC CLIP** — pick an audio track and the clip ranges to extract from a video, then render the music clip.
- **TUTORIAL** — annotate a video with timed events (cuts, speed-up sections and spoken audio comments), generate the spoken audio with the [Piper](https://github.com/rhasspy/piper) TTS engine, and render a finished tutorial video with the narration mixed in and subtitles embedded.

## Features

### Common

- Open a video file (any format `ffprobe`/Qt Multimedia can read) and keep a list of recent videos and recent project files.
- Project files are saved/loaded as JSON, with an automatic re-save every 10 minutes while a project is open.
- Settings dialog (`Edition → Settings`) persisted in `~/.config/rolisteam/ClipSelector.conf`.

### Tutorial mode

- Timeline panel with time ticks and labels, mouse-wheel zoom, and draggable event tokens.
- Three types of timed events placed on the timeline:
  - **Cut** — remove a range of the video.
  - **Speed up** — accelerate a range (user-definable speed factor).
  - **Audio comment** — a spoken note; multi-line text, with the audio generated on demand.
- Piper TTS integration:
  - Text-to-speech for audio comments in English (`en_US-amy-medium`) or French (`fr_FR-mls-medium`).
  - The generated WAV is measured with `ffprobe` and the comment's end time adapts to the real audio duration.
  - Audio files can be regenerated or removed per comment from the side panel.
- Live, effect-aware playback: cuts are skipped, speed-up sections play at the chosen rate, and comment audio starts exactly at its token position.
- Side panel with a table of all timed events (type / begin / end) and the list of generated audio files.
- SRT subtitle generation from the audio comments.
- Final render with FFmpeg: cuts applied, speed-up sections applied, narration mixed at the right positions, subtitles embedded.

### Music clip mode

- Audio file browser (metadata read with Taglib) to pick the music track.
- Sub-clip table to define the ranges to keep.
- Clip rendering through an external ClipMaker script, with options: *Keep Size*, *Label*, *Merge*, *No music*.

## Requirements

### Build dependencies

| Dependency | Notes |
|---|---|
| CMake ≥ 3.16 | Ninja is the recommended generator |
| Qt 6.11 | Modules: Core, Gui, Quick, QuickControls2, Qml, Multimedia (Test for the unit tests) |
| C++14 compiler | e.g. GCC |
| Taglib | found through pkg-config (`taglib`) |
| libaudiorw | shared library; the build expects `/usr/lib/libaudiorw.so` and headers in `/usr/include` (see `src/libs/core/CMakeLists.txt`) |
| FFmpeg | `ffmpeg` + `ffprobe` in `PATH`; `ffmpeg` needs libx264, aac and the subtitles filter |

### Piper TTS (tutorial mode only)

Piper is optional; without it the tutorial mode works but audio comments cannot be generated. The app drives Piper through a Python script:

- A `gen_audio_files.py` script (see `scripts/gen_audio_files.py`).
- Voice models (`.onnx` + `.onnx.json`) in the **same directory** as the script — `en_US-amy-medium` and `fr_FR-mls-medium` are provided in `scripts/`.
- A Python virtual environment (`.venv`) in the **same directory** as the script, with the `piper` package installed.

The script path is the single source of truth: the model directory and the Python interpreter (`<scriptDir>/.venv/bin/python3`) are derived from it. It can be changed in the settings dialog (`Piper script` field); the derived values are shown there (the Python path is read-only).

## Building

```sh
cmake -B build -G Ninja -DBUILD_TEST=ON
cmake --build build
```

The application binary is `build/src/bin/clipSelector`:

```sh
./build/src/bin/clipSelector
```

## Tests

Unit tests are built when `-DBUILD_TEST=ON` is set:

```sh
cd build
ctest --output-on-failure
```

or run them directly (headless environments):

```sh
QT_QPA_PLATFORM=offscreen ./build/src/tests/tst_model
QT_QPA_PLATFORM=offscreen ./build/src/tests/tst_tutorial
```

Covered areas: the audio file model, the video effect model (roles, JSON serialization), TTS path derivation, FFmpeg command construction, effect-aware playback, and timeline token dragging.

## Repository layout

```
CMakeLists.txt            top-level build
src/
  bin/                    main.cpp + Main.qml (application entry, menus, auto-save)
  libs/
    core/                 C++ core: controllers, models, tutorial engine
      controllers/        main, project, video, music, render, tutorial controllers
      model/              clip, audio file, range, recent-files models
      tutorial/           effects, TTS engine, SRT generator, FFmpeg renderer,
                          effect-aware playback
    views/                QML views (player, timeline, tables, side panel, dialogs)
  tests/                  tst_model, tst_tutorial
scripts/                  Piper TTS script + voice models (en, fr)
assets/                   icons
doc/                      Doxyfile
```

## Project data

- Project files: JSON, saved next to (or anywhere you choose via *Save As*).
- Generated comment audio: `<projectDir>/<videoName>.tutorial/audio/<commentId>/file_00.wav` (falls back to `~/ClipSelectorTutorial.tutorial` when no project directory is set).
- Rendered video and SRT: created by the render step, paths shown in the tutorial view.
