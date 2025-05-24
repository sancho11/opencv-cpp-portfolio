# Interactive Image Scaler

This demo shows how to build a simple interactive GUI with OpenCV in C++ to scale an image up or down in real time using sliders (trackbars) and to save the result on demand. Rather than focusing on build steps (covered in the main README), this document explains the key concepts and how the code works, so you can learn and adapt these techniques in your own projects.

---

## 🚀 Overview

- **Goal:** Load an image, let the user adjust a scale factor and mode (enlarge vs. shrink) via sliders, display the scaled image live, and save it by pressing **“s”**.
- **Key OpenCV modules:**  
  - **HighGUI** (`cv::namedWindow`, `cv::createTrackbar`, `cv::waitKey`)  
  - **ImgProc** (`cv::resize`, interpolation flags)  

---

## 🔑 Concepts Applied

1. **Windows & Trackbars**  
   - Create a named window with `cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE)`.  
   - Attach two trackbars to that window:  
     ```cpp
     cv::createTrackbar("Scale (%)", windowName, &scalePercent, kMaxScale, onScaleChange);
     cv::createTrackbar("Mode (0=up,1=down)", windowName, &scaleType,    kMaxType,    onScaleChange);
     ```
   - Trackbars modify the global variables `scalePercent` and `scaleType` and invoke the callback `onScaleChange` on every change.

2. **Callback Mechanism**  
   - **Signature:** `void onScaleChange(int, void*)`.  
   - Every time a slider moves, OpenCV calls this function so you can re-compute and redisplay your image immediately—no manual polling needed.

3. **Real-Time Resizing**  
   - Compute a **scale factor** based on slider values:  
     ```cpp
     double factor = (scaleType == 0)
                     ? 1.0 + scalePercent / 100.0   // enlarge
                     : std::max(0.01, 1.0 - scalePercent / 100.0);  // shrink
     ```
   - Call `cv::resize(originalImage, currentScaled, cv::Size(), factor, factor, cv::INTER_LINEAR)` to produce the scaled output.

4. **Interpolation Methods**  
   - **`INTER_LINEAR`** is a good general-purpose interpolator for real-time applications:  
     - Smooth results  
     - Fast enough for most CPUs  
   - You can swap it out for `INTER_NEAREST`, `INTER_CUBIC`, etc., to compare edge sharpness vs. performance.

5. **Event Loop & Keyboard Interaction**  
   - Use a loop with `cv::waitKey(20)` to process window events and read key presses.  
   - Exit on **Esc** (`key == 27`), save on **‘s’** or **‘S’** (`key == 's'`).

6. **Saving Images**  
   - The helper `saveImageOrExit(path, image, params)` wraps `cv::imwrite` and reports errors.  
   - You supply PNG compression level via `{ cv::IMWRITE_PNG_COMPRESSION, 3 }` for a balance of size vs. quality.

---

## 🔍 How It Works

1. **Startup**  
   - Determine input/output paths (defaults if none given).  
   - Load the original image or exit with an error message.

2. **UI Setup**  
   - Create the display window.  
   - Create two trackbars bound to `scalePercent` and `scaleType`, both calling `onScaleChange`.

3. **Initial Display**  
   - Call `onScaleChange(0, nullptr)` once to show the unmodified image.

4. **Interactive Loop**  
   - In `while(true)`, poll for key events.  
   - If the user adjusts a slider, the callback rescales and re-shows the image automatically.  
   - If **‘s’** is pressed, the current scaled image is saved.  
   - If **Esc** is pressed, the loop breaks and the program ends.

---

## 🛠️ Extensions & Learning Paths

- **Multiple interpolation options:** add a third trackbar to switch between `INTER_NEAREST`, `INTER_LINEAR`, `INTER_CUBIC`, etc., and observe their effects.  
- **Non-uniform scaling:** introduce separate sliders for X and Y scales to stretch the image arbitrarily.  
- **Aspect-ratio locks:** learn how to enforce constant aspect ratios by linking slider values.  
- **Real-time performance metrics:** overlay current frame processing time (in ms) on the displayed image using `cv::getTickCount()`.

---

By dissecting this simple interactive tool, you’ll gain hands-on experience with OpenCV’s GUI utilities, callbacks, and image resizing—all fundamental for building more sophisticated computer-vision applications. Enjoy experimenting!