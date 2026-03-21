# Deepfake Detector

Real-time deepfake / authenticity detection using openFrameworks + MediaPipe.

Work in progress — right now we have face tracking working with MediaPipe's 468-point face mesh. Detection algorithms are coming next.

## Setup

### 1. Get openFrameworks

Download **oF 0.12.1** from [openframeworks.cc/download](https://openframeworks.cc/download/) and unzip it.

- **macOS**: we used `~/Documents/of_v0.12.1_osx_release/`
- **Linux**: `~/openFrameworks/` or wherever you want

If you're on Linux, you also need to install dependencies and compile oF first:
```bash
cd openFrameworks/scripts/linux/ubuntu
sudo ./install_dependencies.sh
cd ..
./compileOF.sh -j4
```

### 2. Install addons

We need two addons. Clone them into the oF addons folder:

```bash
cd <of path>/addons
git clone https://github.com/design-io/ofxMediaPipePython.git
git clone https://github.com/kylemcdonald/ofxCv.git
```

### 3. Set up MediaPipe

This is the annoying part. The addon uses Python 3.11 + MediaPipe via pybind11, so there's some setup involved. Luckily there's an install script that handles most of it:

```bash
cd ofxMediaPipePython
bash InstallMediaPipe.sh
```

It'll create a conda environment, install python 3.11, and copy the libs into the addon.

**If you're on Apple Silicon:** the script tries to install mediapipe 0.10.9 which doesn't exist for arm64. It'll fail on that step but everything else still gets set up. Just run this after:
```bash
conda activate mediapipe
pip install mediapipe
```

### 4. Clone this repo

```bash
cd <of path>/apps/myApps/
git clone https://github.com/Mista-Kev/deepfake-detector.git
```

### 5. Copy the face model

MediaPipe needs a model file to do face detection. It comes with the addon, you just need to put it where the app can find it:

```bash
cd deepfake-detector
mkdir -p bin/data
cp <of path>/addons/ofxMediaPipePython/tasks/face_landmarker_v2_with_blendshapes.task bin/data/
```

### 6. Build & run

```bash
make -j4
```

**macOS only** — you need to copy the python dylib into the app bundle, otherwise it crashes on launch:
```bash
cp <of path>/addons/ofxMediaPipePython/libs/python/lib/osx/libpython3.11.dylib bin/deepfake-detector.app/Contents/MacOS/
```

Then:
```bash
make RunRelease
```

If everything went right you should see your webcam with green bounding boxes around faces and blue landmark dots.

### Camera issues on macOS

Some stuff that tripped us up:

- **Permission dialog**: First time running, macOS asks for camera access. You might need to restart the app after granting it.
- **Device ID**: `cam.setDeviceID(0)` is the built-in webcam. USB cameras are usually `1` or `2`, just trial and error.
- **Black screen**: Check System Settings > Privacy > Camera and make sure your terminal has access.
- **Resolution**: We request 1280x720 but your camera might give something different. The app scales either way so it's fine.

### Linux

On Linux you might need to set the library path before running:
```bash
export LD_LIBRARY_PATH=<of path>/addons/ofxMediaPipePython/libs/python/lib/linux64:$LD_LIBRARY_PATH
```
## FFT Analyzer — Limitations

The FFT analyzer computes the ratio of high-frequency energy in the
face crop's magnitude spectrum. GAN-generated faces sometimes exhibit
elevated high-frequency artifacts from transposed convolution upsampling,
which this tries to detect.

**Known limitations:**
- Thresholds are heuristic and not trained on a labelled dataset
- Results vary significantly with video codec, resolution, and compression
  level — a heavily compressed real face can score similarly to a GAN face
- Works best on high-quality, uncompressed input
- Should be treated as a weak signal only; weight it lower than blink
  and jitter analysis until properly calibrated
- A production implementation would replace the threshold logic with a
  classifier trained on a labelled real/fake dataset (e.g. FaceForensics++)
