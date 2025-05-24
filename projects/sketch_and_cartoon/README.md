# Pencil Sketch & Cartoon Filter

This project demonstrates two complementary image-processing techniques using OpenCV in C++:

- Pencil Sketch: Extracts linear, hand-drawn–style strokes by high-pass filtering the brightness channel.
- Cartoon Effect: Blends a color-smoothed version of the image with the sketch strokes to produce a stylized, cartoon-like output.

---

Core Concepts

1. Pencil Sketch via High-Pass Filtering

   1. Brightness Extraction: Convert the input from BGR to HSV and isolate the V (value) channel, which encodes intensity.
   2. Low-Pass (Gaussian) Blur: Smooth the V channel to capture broad lighting variations (low frequencies).
   3. High-Pass Approximation: Subtract the original V from its blurred version. This highlights rapid intensity changes—ideal for revealing edges and stroke-like patterns.
   4. Thresholding: Apply a binary inverse threshold so that dark strokes appear white on a black background, yielding a clean sketch mask.

   This sequence mimics hand-drawn pencil strokes by enhancing edges and suppressing flat regions.

2. Cartoonification via Bilateral Filtering & Mask Blending

   1. Color Smoothing: Use a bilateral filter, which reduces color noise while preserving sharp edges. Unlike Gaussian blur, bilateral filtering respects edge boundaries by combining spatial and color proximity.
   2. Mask Inversion: Invert the sketch mask so that stroke regions become zeros and flat areas become ones.
   3. Mask Application: Apply the inverted mask to the smoothed image, setting non-edge regions to black. The remaining strokes overlay the color regions, producing a cartoon effect.

   Bilateral filtering softens textures and colors without blurring important contours, then the sketch mask emphasizes those contours as dark strokes.

---

Code Structure

- Utility Functions (at top of source)
  - loadImageOrExit(), saveImageOrExit(), showImage()
  - Handle file I/O, error checking, and window display.

- computeSketchMask(const cv::Mat&)
  - Implements the high-pass sketch generation.

- pencilSketch(const cv::Mat&)
  - Returns the binary sketch image (calls computeSketchMask).

- cartoonify(const cv::Mat&)
  - Smooths colors with cv::bilateralFilter
  - Generates and inverts the sketch mask
  - Blends mask and color image

- main()
  1. Constructs file paths from DATA_DIR and relative constants.
  2. Loads the source image in color mode.
  3. Calls pencilSketch and cartoonify.
  4. Displays both results in resizable windows.
  5. Saves outputs (resultSketch.png, resultCartoon.png).

---

Learning Points

- Frequency Separation: How Gaussian blur and subtraction isolate edges (high frequencies) from smooth regions (low frequencies).
- Edge-Preserving Smoothing: Why bilateral filters are preferred in stylization pipelines.
- Masking Techniques: Inverting and applying masks to selectively retain or remove image regions.
- Pipeline Design: Structuring image transformations into clear, reusable functions for readability and extension.

---

Extensions & Experiments

- Parameter Tuning: Experiment with Gaussian kernel size, sigma values, and threshold to achieve different sketch styles.
- Color Quantization: Apply color reduction (e.g., k-means clustering) before bilateral filtering to create more "flat" cartoon regions.
- Adaptive Thresholding: Use adaptive methods for sketch masks to handle variable lighting across the image.
- Video Processing: Extend the pipeline to work frame by frame on a video stream for live cartoon previews.

Use this code as a foundation to explore stylized image filters, pipeline modularity, and edge-based processing techniques in computer vision.