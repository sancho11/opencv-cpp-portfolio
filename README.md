# 🧠 OpenCV Portfolio

Welcome to my OpenCV portfolio! This repository showcases a curated set of computer vision projects I've developed using OpenCV (C++). Each project demonstrates practical applications of real-time image processing, feature detection, face filtering, segmentation, and stylization techniques.

🎯 **All projects are self-contained with code, documentation, and demos.**

---

## 🕶️ [Sunglasses++](https://github.com/sancho11/sunglassespp)
**Face Filter & Augmented Reality Effects**

A real-time face filter pipeline that overlays dynamic sunglasses, mustaches, and lens effects using C++ and OpenCV.

### Highlights:
- **Face Tracking**: Haar cascade + MedianFlow MultiTracker.
- **Binary Masks**: Isolates lens and frame regions for independent blending.
- **Reflection Texture**: Dynamic lens reflections using contrast-enhanced textures.
- **Alpha Blending**: Custom opacity control for realistic overlay.
- **Lens Flare/Scratch**: Gradient-based texture enhancement in HSV space.
- **Mustache Placement**: Uses color thresholding and centroid estimation for facial anchoring.
- **Interactive UI**: Real-time parameter tuning with OpenCV trackbars.

---

## ✏️ [Sketch & Cartoon](https://github.com/sancho11/sketch_and_cartoon)
**Stylized Image Processing**

Applies a hand-drawn pencil sketch and cartoon effect using classical image filters and transformations.

### Highlights:
- **Color Space Transformation**: Converts BGR to HSV to process intensity independently.
- **Pencil Sketching**: High-pass filtering on the V channel with thresholding.
- **Cartoon Effect**: Bilateral filtering for smooth, flattened color regions.
- **Edge Enhancement**: Combines sketch edges with smoothed colors for stylized output.

---

## 💆‍♀️ [Skin Smoothing](https://github.com/sancho11/skin_smoothing)
**Automatic Retouching & Blemish Removal**

Performs semantic skin smoothing, blemish detection, and facial enhancement with realistic blending.

### Highlights:
- **Face & Eye Detection**: Region-aware facial analysis using Haar cascades.
- **Skin Color Modeling**: HSV histogram modeling from face region.
- **Adaptive Segmentation**: Refines skin mask with morphological ops and GrabCut.
- **Texture-Aware Repair**: Detects and replaces blemishes using seamless cloning.
- **Natural Smoothing**: Bilateral filtering for subtle texture correction.

---

## 🎬 [Chroma Key Compositor](https://github.com/sancho11/chroma_key)
**Real-Time Green Screen Removal**

Implements interactive chroma keying to composite subjects over new backgrounds using HSV-based segmentation.

### Highlights:
- **Key Color Sampling**: Click-to-select green screen reference from video stream.
- **HSV Segmentation**: Robust tolerance ranges for color keying.
- **Soft Masking**: Blurred, inverted binary mask for smooth edge transitions.
- **Spill Correction**: Automatic green spill suppression with color redistribution.
- **Interactive UI**: Trackbars to tweak tolerance, softness, and blending modes.

---

## 📄 [Document Scanner](https://github.com/sancho11/document_scanner)
**Perspective-Corrected Document Capture**

Transforms angled photos of paper documents into clean, top-down scans using contour detection and homography.

### Highlights:
- **V Channel Emphasis**: Focuses on brightness for better edge clarity.
- **Canny Edge + Morphology**: Detects document contours and strengthens borders.
- **Contour Filtering**: Finds the largest 4-sided polygon as the paper outline.
- **Perspective Transform**: Computes warp matrix with `getPerspectiveTransform()`.
- **Side-by-Side View**: Shows original vs. scanned output for easy validation.

---

## 📫 Contact

- GitHub: [sancho11](https://github.com/sancho11)
- Email: [enmanuel.sancho11@gmail.com]
- LinkedIn: [Enmanuel Sancho](https://www.linkedin.com/in/sancho11/)

---

⭐ If you find these projects useful or interesting, feel free to star the repositories or reach out!