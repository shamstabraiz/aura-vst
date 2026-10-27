# Aura Compressor (VST3 & Standalone Audio Plugin)

A precision dynamic range compressor built with **C++** and **JUCE 8**. Features analog-modeled ballistics, continuous soft-knee adjustment, sidechain high-pass filtering, real-time gain reduction & peak level metering, and an interactive dynamic transfer curve visualizer.

---

## ✨ Features

- **Dynamic Range Compression Engine**:
  - **Threshold**: `-60.0 dB` to `0.0 dB` with high-resolution continuous control.
  - **Ratio**: `1.0:1` (unity) to `20.0:1` (brickwall limiting).
  - **Attack**: `0.1 ms` to `200.0 ms` (musical log-skew curve).
  - **Release**: `10.0 ms` to `1000.0 ms` (smooth decay).
  - **Soft Knee**: `0.0 dB` (hard knee) to `24.0 dB` (ultra-smooth progressive knee).
  - **Makeup Gain**: `-12.0 dB` to `+24.0 dB`.
  - **Parallel Mix (Wet/Dry)**: `0%` to `100%` for NYC-style parallel compression.
  - **Sidechain HPF**: `20 Hz` to `500 Hz` 2nd-order Butterworth high-pass filter on the detector signal to prevent low-end bass/kick pumping.
  - **Detector Modes**: Toggle between **Peak** (transient-focused) and **RMS** (smooth energy-averaged) detection.
  - **Bypass**: True click-free processing bypass.

- **Modern Studio UI**:
  - **Transfer Curve Display**: Real-time 2D curve visualization depicting the static input/output curve, knee curvature, threshold marker, and an animated bouncing operating point showing current compression in action.
  - **Gain Reduction Meter**: Responsive vertical GR meter (`0 dB` to `-30 dB`) with instant attack and smooth release decay.
  - **Stereo Level Meters**: Input and Output peak meters (`-60 dB` to `+6 dB`) with clip indicator.
  - **Custom Rotary Knobs**: Glowing value arcs, center needles, accent-coded colors, and numeric value readouts.

---

## 📁 Project Structure

```
vst/
├── CMakeLists.txt                 # CMake project configuration
├── JUCE/                          # JUCE framework submodule/source
├── Source/
│   ├── Audio/
│   │   ├── CompressorDSP.h        # Core compressor mathematical DSP engine
│   │   └── SidechainFilter.h      # 2nd-order Butterworth sidechain filter
│   ├── UI/
│   │   ├── CustomLookAndFeel.h    # Dark modern studio styling & knob graphics
│   │   ├── CustomLookAndFeel.cpp
│   │   ├── RotaryKnob.h           # Custom knob component with labels & readouts
│   │   ├── MeterComponent.h       # GR and Level peak meters
│   │   └── TransferCurveComponent.h # 2D Transfer curve visualizer
│   ├── PluginProcessor.h          # AudioProcessor & APVTS parameter layout
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h             # GUI layout & 30fps meter timer
│   └── PluginEditor.cpp
└── Tests/
    └── CompressorTests.cpp        # DSP validation unit test suite
```

---

## 🚀 Built Binaries & Installation

The binaries have been compiled in **Release mode**:

| Format | File Path | Default Linux Search Path |
|---|---|---|
| **LV2 Plugin Bundle** *(Native for LMMS)* | `build/AuraCompressor_artefacts/Release/LV2/AuraCompressor.lv2` | `~/.lv2/AuraCompressor.lv2` |
| **VST3 Plugin Bundle** | `build/AuraCompressor_artefacts/Release/VST3/AuraCompressor.vst3` | `~/.vst3/AuraCompressor.vst3` |
| **Standalone App** | `build/AuraCompressor_artefacts/Release/Standalone/AuraCompressor` | — |

### 1. Standalone Application (Test without a DAW)
You can run the standalone compressor directly to process microphone, line-in, or audio interface input live with the full UI:
```bash
./build/AuraCompressor_artefacts/Release/Standalone/AuraCompressor
```
*(Go to `Options -> Audio Settings` inside the app to select your soundcard/ALSA/PulseAudio/JACK, sample rate, and buffer size).*

### 2. Audio File Test & Audition Utility
Generate real dynamic audio and process it directly through the compressor:
```bash
# Run audio processing demo
./build/ProcessAudioFile

# Listen to the results on Linux
aplay Tests/output_compressed.wav
# or
ffplay -nodisp -autoexit Tests/output_compressed.wav
```

### 3. Using in DAWs (Reaper, Bitwig, Ardour, Ableton, FL Studio, Carla)
The plugin is already installed to `~/.vst3/AuraCompressor.vst3`.
1. Open your DAW (e.g. **Reaper**, **Bitwig**, **Ardour**, or **Carla**).
2. Ensure `~/.vst3` is in your DAW's VST3 search path.
3. Trigger a **Rescan Plugins**.
4. Insert **Aura Compressor** (`AuraDSP`) onto any audio track or master bus.

---

## 🪟 How to Build for Windows

JUCE and Steinberg VST3 on Windows require the **Microsoft Visual C++ (MSVC)** toolchain (due to COM interface ABI requirements for DAWs like Cubase, Ableton, FL Studio, and Studio One).

### Option A: Automated GitHub Actions (Easiest — No Windows PC Needed)
A ready-to-run GitHub Actions workflow is included at [`.github/workflows/build.yml`](file:///home/shams/Desktop/vst/.github/workflows/build.yml):
1. Push this repository to GitHub.
2. Go to the **Actions** tab.
3. The build runs automatically on `windows-latest` with Visual Studio 2022 and CMake.
4. Download the ready-to-use `AuraCompressor-Windows-VST3.zip` containing `AuraCompressor.vst3`.

### Option B: Building on a Windows Machine
If building directly on a Windows PC (with Visual Studio 2022 Community or Build Tools installed):

1. Open **Command Prompt** or **PowerShell**:
   ```powershell
   cd vst
   cmake -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release --parallel
   ```
2. The outputs will be located at:
   - **VST3**: `build\AuraCompressor_artefacts\Release\VST3\AuraCompressor.vst3`
   - **Standalone**: `build\AuraCompressor_artefacts\Release\Standalone\AuraCompressor.exe`
3. Copy `AuraCompressor.vst3` to the standard Windows VST3 directory:
   `C:\Program Files\Common Files\VST3\`

---

## 🛠️ Rebuilding & Testing on Linux

To rebuild the project on Linux:
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

To run the DSP unit test suite:
```bash
./build/CompressorTests
```
