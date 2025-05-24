# Panorama Stitching

This project demonstrates how to assemble multiple overlapping images into a single panoramic view using OpenCV’s high-level stitching API. Instead of focusing on build instructions (handled in the root README.md), this document explains the algorithms and C++ techniques applied, making it easier for learners and code reusers to understand the implementation.

1. Directory Traversal with std::filesystem
- Goal: Automatically gather all image files from a specific folder.
- Key Techniques:
  - std::filesystem::directory_iterator: Iterates through entries in a directory.
  - File filtering by extension: Check each entry’s path().extension() against the target (e.g. .jpeg).
  - Error handling: Throw a std::runtime_error if the target directory is missing or invalid, so the application fails fast with clear feedback.

2. Robust Image Loading
- Goal: Load only valid images, skipping corrupted or unreadable files.
- Key Techniques:
  - cv::imread: Reads an image into a cv::Mat. If empty, the image is invalid.
  - Graceful skipping: Log a warning and continue when encountering an unreadable image, ensuring the pipeline isn’t halted by a single bad file.

3. Consistent Ordering
- Goal: Ensure the stitcher processes images in a predictable sequence.
- Key Technique:
  - std::sort on file paths: Sorts the vector of std::filesystem::path so that images are stitched in filename order, which usually corresponds to their spatial arrangement.

4. Panorama Stitching with OpenCV
- Goal: Combine a series of images into one seamless panorama.
- Key Component:
  - cv::Stitcher:
    - Modes: We use cv::Stitcher::PANORAMA which is optimized for perspective panoramas (good for scenes taken from a single rotation point).
    - Workflow:
      1. Feature detection and matching: Finds keypoints (e.g. ORB/SIFT) and matches them across image pairs.
      2. Camera parameter estimation: Computes homographies to align images.
      3. Seam finding and blending: Determines optimal seams and blends overlapping regions to hide transitions.
    - Status codes: The stitch() method returns a cv::Stitcher::Status. Checking against Stitcher::OK ensures success, while other codes (e.g. ERR_NEED_MORE_IMAGES) indicate specific failures.

5. Saving the Result
- Goal: Persist the final panorama to disk with guaranteed success.
- Key Technique:
  - cv::imwrite wrapped in a helper: Attempts to write the cv::Mat to file and exits on failure, giving a clear error message if there’s an I/O issue.

6. Error Handling and User Feedback
- The code exits with distinct messages for each failure mode:
  - Missing or empty directory.
  - No valid images found or less than two images.
  - Failure during stitching.
  - File write errors.
- This approach ensures any user or developer running the code immediately knows what went wrong and where to look.

Learning Takeaways
- Modern C++: Leveraging the <filesystem> library for portable directory operations.
- OpenCV abstraction: Using the high-level Stitcher interface abstracts away low-level feature detection and blending details, letting you focus on pipeline orchestration.
- Defensive programming: Combining exception-based directory checks with return-code validation for image operations leads to robust tools.
