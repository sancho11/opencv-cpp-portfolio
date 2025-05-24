# Autofocus_evaluator

This module demonstrates automatic evaluation of frame sharpness (focus) in a video sequence using multiple image-based metrics. It's designed to help learners explore how different mathematical measures can quantify focus quality and compare their computational costs.

Overview of the Pipeline

1. Video Input & ROI  
   - Load a video file (defaulting to data/focus-test.mp4).  
   - Optionally define a rectangular Region of Interest (ROI) to restrict analysis to a sub-area of each frame.

2. Focus Metrics Computation  
   For each frame (or its ROI), four distinct metrics are calculated:
   - Absolute Variance of the Laplacian  
   - Sum of Modified Laplacian (Lovlac)  
   - Local Variance (sliding window)  
   - Variance of Gradient Magnitude

3. Performance Timing  
   - Measure per-frame computation time for each metric using high-resolution timers.  
   - Accumulate total durations to compute average milliseconds per frame.

4. Best-Focus Frame Selection  
   - Track the frame that maximizes each metric over the entire video.  
   - Store both the metric value and corresponding frame index.

5. Visualization  
   - Concatenate the four best-focus frames into a 2×2 image grid.  
   - Display the result window for qualitative comparison.

Core Concepts

1. Laplacian-Based Focus (Frequency Content)
- Theory: The Laplacian operator measures second-order spatial derivatives (edges and fine details). A sharper (well-focused) image has stronger high-frequency content, resulting in larger Laplacian responses.
- Implementation: Convert to HSV, extract the V channel, apply cv::Laplacian (3×3 kernel), square each response, and sum all values.
  [varAbsLaplacian = sum((∇² I_V)²)]

2. Modified Laplacian Sum (Directional Focus)
- Theory: Lovlac filter applies a three-tap directional Laplacian in both x and y, emphasizing edges along each axis. Absolute sums quantify overall directional detail.
- Implementation: Convolve V channel with kernels [ -1, 2, -1 ] horizontally and vertically, take absolute values, and sum both directions.
  [sumModifiedLaplacian = sum(|L_x| + |L_y|)]

3. Local Variance (Texture Contrast)
- Theory: Local variance computes the spread of pixel intensities within a sliding window. Areas in focus exhibit higher local contrast between neighboring pixels.
- Implementation:
  1. Normalize V channel to [0,1].
  2. Compute mean within each 3×3 window (filter2D).
  3. For each pixel, sum squared differences from the local mean.
  4. Compute the variance of these local variances across the ROI.
  [varLocal = Var({σ²_W(i,j)})]

4. Gradient Magnitude Variance (Edge Strength)
- Theory: The gradient magnitude combines first-order derivatives in x and y. Focused images have stronger overall gradients.
- Implementation: Compute Sobel derivatives on V channel, square and sum them to obtain magnitude square, then sum across the image.
  [varGradMagnitude = sum(G_x² + G_y²)]

Interpreting Results

- Numeric Scores: Higher values indicate sharper frames. Each metric may peak on different frames—comparing them illustrates metric sensitivity.
- Performance Trade-offs: Average computation times help decide which metric balances accuracy and speed for real-time applications.

Extending & Usage Tips

- Choosing ROI: Restrict to areas of interest (e.g., faces) to avoid background noise.
- Metric Selection: For real-time autofocus, the simple Laplacian variance often suffices; more complex metrics yield marginal gains at higher cost.
- Batch Analysis: Integrate with automated scripts to process multiple videos and log focus statistics.
- Visualization: Adapt displayImage to save results to disk for headless execution.

---
This README focuses on the theoretical underpinnings and code structure. Compilation and runtime instructions are provided in the main repository README.