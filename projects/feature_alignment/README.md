# Feature Alignment

## Overview
This tool demonstrates how to correct misaligned color channels in a scanned or stacked image by using feature-based alignment. It splits a single grayscale image (containing three stacked channels) into B, G and R layers, finds matching points between each channel, computes homographies with RANSAC, warps the outer channels onto the green middle channel, and finally merges them back into a full-color image for comparison.

## Key Concepts
- **Stacked-channel representation**  
  Treats one grayscale image as three separate color channels arranged vertically.
- **ORB feature detector & descriptor**  
  Fast, rotation-invariant keypoint detection and binary descriptors (ORB = Oriented FAST and Rotated BRIEF).
- **Brute-force Hamming matching**  
  Matches ORB descriptors using binary Hamming distance, then filters the best matches by a fixed percentage.
- **RANSAC homography estimation**  
  Robustly fits a 3×3 projective transform while rejecting outliers.
- **Perspective warping**  
  Applies the homography to realign the blue and red channels onto the green channel’s coordinate frame.
- **Channel merging & visualization**  
  Recombines aligned channels into an RGB image and displays intermediate results in grid layouts.

## Workflow

1. **Load Input**  
   - Read a grayscale image that actually contains three equally-sized regions representing blue, green and red channels stacked vertically.

2. **Split Channels**  
   - Compute height `h = total_rows / 3` and extract three `cv::Mat` sub-images:  
     - Top region → Blue  
     - Middle region → Green  
     - Bottom region → Red  

3. **Detect & Describe Features**  
   - Use `cv::ORB::create(maxFeatures=20000)` to detect keypoints and compute binary descriptors on each channel.

4. **Match & Filter**  
   - Create a BRUTEFORCE_HAMMING matcher.  
   - Match green→blue and green→red descriptors.  
   - Sort matches by Hamming distance and keep only the top 0.5%.

5. **Estimate Homography**  
   - Convert matched keypoints to point-pairs.  
   - Use `cv::findHomography(..., RANSAC)` to compute robust homography matrices `H_B→G` and `H_R→G`.

6. **Warp Channels**  
   - Apply `cv::warpPerspective` with each homography to map blue and red onto the green plane.

7. **Merge & Compare**  
   - Reconstruct two RGB images:  
     - **Original**: direct merge of unaligned B, G, R.  
     - **Aligned**: merge of warped B, original G, warped R.  
   - Display side by side for visual comparison.

## Function Breakdown

- **`loadImageOrExit(path)`**  
  Loads an image in grayscale mode and exits on failure.
- **`displayGrid(images, rows, cols, winName)`**  
  Arranges multiple `cv::Mat` in a grid and shows them in one HighGUI window.
- **`detectAndCompute(img, keypoints, descriptors)`**  
  Runs ORB to find up to 20 000 keypoints and associated descriptors.
- **`matchAndFilter(desc1, desc2, matches)`**  
  Matches two descriptor sets, sorts by distance, then retains the best 0.5%.
- **`computeHomography(kp1, kp2, matches)`**  
  Extracts matched point coordinates and computes a RANSAC homography.

## Learning Outcomes
- How feature matching can drive geometric alignment across image channels.
- Tuning ORB and match-filter parameters for reliable correspondence.
- Applying RANSAC to reject bad matches and compute stable transforms.
- Using perspective warping to correct spatial misregistrations.
- Organizing OpenCV pipelines into modular, reusable functions.

## Dataset
- A sample image `emir.jpg` (stacked B / G / R) is provided.  
- You can swap in your own stacked-channel images following the same layout to experiment with different inputs.