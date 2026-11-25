# Vibe Saturator

A VST3 saturator with six waveshaping curves, 4x oversampling, and a live transfer-curve display.

**Controls**

| Control | Range | What it does |
|---|---|---|
| Curve | 6 types | Analog Clip, Soft Sine, Medium, Hard, Sinoid Fold, Digital Clip |
| Drive | -36 to +36 dB | Gain into the curve |
| Output | -36 to +12 dB | Level after dry/wet mix |
| Dry/Wet | 0 to 100 % | Blend of clean and saturated signal |
| DC Filter | on/off | 10 Hz high-pass on the saturated signal |

## Get a Windows VST3 (no programming tools needed)

GitHub builds it for you in the cloud, for free.

1. Make a free account at github.com, then click **New repository**. Name it anything (e.g. `vibe-saturator`), keep it **Public** (free build minutes are unlimited for public repos), and create it.
2. On the new repo page, click **uploading an existing file**. Drag in everything from this folder: `CMakeLists.txt`, the `Source` folder, the `.github` folder, and `.gitignore`. Click **Commit changes**.
   - If the `.github` folder doesn't upload, use **Add file → Create new file**, type `.github/workflows/build.yml` as the name, and paste in the contents of that file.
3. Click the **Actions** tab. A build called "Build VST3 (Windows)" starts by itself. It takes roughly 5 to 10 minutes. A green check means it worked.
4. Click the finished run, scroll to **Artifacts**, and download `VibeSaturator-VST3-Windows`. Unzip it.
5. Copy the `Vibe Saturator.vst3` folder into `C:\Program Files\Common Files\VST3`.
6. In FL Studio: **Options → Manage plugins → Find installed plugins** (turn on "Verify plugins" for the first scan). Then find **Vibe Saturator** in the plugin database and add it to a mixer slot.

If the build shows a red X, open the failed run, copy the error text, and send it to me. I'll fix it.

## Building locally instead (optional)

Needs Visual Studio 2022 with the "Desktop development with C++" workload and CMake:

```
cmake -B build
cmake --build build --config Release
```

The plugin appears in `build/VibeSaturator_artefacts/Release/VST3/`.

## Notes

- The plugin adds a small fixed latency from oversampling. FL Studio compensates automatically.
- Curves are symmetric, so they add odd harmonics. Sinoid Fold folds the waveform back on itself once it passes full scale, which gets metallic at high drive.
- At extreme drive (especially Sinoid Fold above about +24 dB) the output can briefly exceed 0 dBFS after oversampling. Pull the Output knob down a few dB, or put a limiter after it.
- This isn't a copy of Ableton's code. It's an original take on the same idea, so expect it to sound similar, not identical.
