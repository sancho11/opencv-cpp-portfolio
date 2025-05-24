# Chroma Key Compositor

This project implements an interactive chroma-key compositor using OpenCV in C++. It demonstrates fundamental techniques for real-time green-screen removal and compositing, with hands-on controls to explore each stage of the process.

---

## Key Concepts Covered

1. **Key Color Sampling**  
   - Click callbacks to pick the chroma key color from the live video frame.  
   - Averaging in a small patch to reduce noise and obtain a stable reference color.

2. **Color Space Transformation**  
   - Converting BGR frames to HSV for robust color-based segmentation.  
   - Calculating a hue–saturation–value tolerance range to isolate pixels near the key color.

3. **Binary Mask Generation**  
   - Creating a hard mask via `cv::inRange()` in HSV space.  
   - Understanding how hue wraps around and why delta calculations must clamp correctly.

4. **Mask Softening**  
   - Inverting and blurring the binary mask to produce a smooth transition edge.  
   - Converting to a floating-point mask in [0..1] to enable soft blending.

5. **Green Spill Correction**  
   - Detecting green spill by comparing the green channel to the maximum of blue and red channels.  
   - Attenuating spill proportionally and redistributing color to preserve natural lighting.

6. **Compositing Techniques**  
   - **Simple Soft Blend**: Per-pixel linear interpolation between foreground and background using the soft mask.  
   - **Seamless Cloning**: OpenCV’s `seamlessClone()` for advanced Poisson blending to hide boundary artifacts.

7. **Interactive Controls**  
   - Trackbars for adjusting tolerance, softness radius, spill-correction strength, and blend mode at runtime.  
   - Real-time feedback to observe how each parameter impacts the final composite.

8. **Processing Pipeline**  
   1. Capture and display the current frame for sampling.  
   2. Generate a binary mask in HSV.  
   3. Soften the mask for smooth edges.  
   4. Correct green spill to maintain color fidelity.  
   5. Composite the corrected foreground over the background using the chosen method.  
   6. Loop the video seamlessly and allow parameter tuning on the fly.

---

## How to Learn from This Code

- **Inspect the Callback Functions**: See how mouse and trackbar events are registered and routed to update global parameters.
- **Study `createKeyMask()`**: Observe how HSV thresholds are computed from a sampled key color.
- **Examine `softenMask()`**: Learn Gaussian blur techniques to smooth binary edges.
- **Analyze `correctGreenSpill()`**: Understand per-channel operations and mask-based blending for spill suppression.
- **Compare Blending Modes in `compositeFrame()`**: Contrast the math behind simple alpha blending and Poisson-based seamless cloning.

Whether you’re building your own green-screen tool or simply exploring advanced OpenCV pipelines, this project provides a clear, interactive demonstration of each core concept.

---

> **Tip**: Experiment with different background images and video resolutions to see how mask size and softness settings affect performance and visual quality.
