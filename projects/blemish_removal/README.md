# Blemish Removal Tool

An interactive blemish removal application built with OpenCV, illustrating how to combine texture analysis and seamless cloning to clean up facial imperfections. This README focuses on the underlying concepts and workflow, serving as a guide for learners and practitioners.

---
Key Concepts

1. Texture Variance Estimation
- We convert each candidate patch to HSV color space and apply a Laplacian filter on the V (value) channel.
- Squaring and summing the Laplacian response yields a measure of texture variance: smoother regions have lower variance.

2. Patch Selection Strategy
- For a user-clicked blemish center, possible source patches are sampled in eight compass directions at twice the patch radius.
- Each patch’s variance is computed; the patch with minimal variance (i.e., the smoothest background) is chosen for cloning.

3. Seamless Cloning
- Using OpenCV’s cv::seamlessClone, the selected patch is blended into the blemish region.
- A circular mask defines the blending region, ensuring a natural transition without harsh edges.

4. Interactive GUI with Mouse Callbacks
- A resizable window listens for left-click events. Each click triggers patch selection and cloning.
- Keyboard controls:
  - C: Reset to the original image.
  - Esc: Exit the application.

---
Workflow Overview

1. Load Image: Robust I/O functions ensure clean loading and saving of images.
2. Display Window: A named, resizable window presents the current state.
3. Handle Clicks: On each click, compute and select the best patch, then apply seamless cloning in-place.
4. User Controls: Reset or exit based on key presses.

---
Code Highlights

- computePatchVariance(patch): Demonstrates how to quantify texture using second-order derivatives.
- selectBestPatch(image, center, radius): Encapsulates the search over multiple directions and variance comparison.
- onMouse(...) Callback: Shows the integration of GUI events with image processing routines.
- Separation of Concerns: Image I/O, variance computation, patch selection, and interaction logic are modularized for clarity.

---
Learning Takeaways

- Practical use of Laplacian filters to guide algorithmic decisions.
- Hands-on experience with OpenCV’s Photo module (seamlessClone).
- Designing user-friendly tools: clear controls, feedback loops, and reset functionality.

---
Potential Extensions

- Experiment with adaptive patch radius or non-circular masks.
- Replace variance metric with gradient magnitude or entropy.
- Automate blemish detection via simple thresholding or blob detection prior to manual clicking.
- Integrate color correction steps to handle lighting differences between source and target.

