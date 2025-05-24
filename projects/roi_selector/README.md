# ROI Selector
============
Interactive ROI (Region of Interest) selector that lets you draw a rectangle over an image window and save the cropped region as a separate file. This document explains the OpenCV concepts and code patterns used to help learning and reuse.

CONCEPTS COVERED
----------------
1. Mouse Callbacks & Event Handling
   - Register callback with cv::setMouseCallback
   - Handle events EVENT_LBUTTONDOWN, EVENT_MOUSEMOVE, EVENT_LBUTTONUP to track mouse

2. Drawing Shapes
   - Use cv::rectangle to overlay guide rectangle
   - Clone source image to preserve original

3. Coordinate Normalization
   - Ensure start and end points define valid top-left / bottom-right
   - Clamp ROI to image bounds using roi &= cv::Rect(0,0,cols,rows)

4. Image Cropping & Saving
   - Extract sub-region: cv::Mat cropped = image(roi).clone()
   - Save with cv::imwrite, handle failures

5. Simple GUI Loop
   - Display frames with cv::imshow
   - Poll keyboard with cv::waitKey

CODE STRUCTURE
--------------
- loadImageOrExit / saveImageOrExit: wrappers for imread/imwrite with error handling
- MouseState struct: holds original image, display copy, start/end points, drawing flag, output path
- onMouse callback: switches on event type to handle drawing and saving
- main loop: loads image, sets callback, displays instructions, loops until ESC or ENTER

HOW IT WORKS
-------------
1. Startup: load image, clone for display
2. Interaction: press+hold left mouse to mark corner, drag to define rectangle, release to save ROI
3. Exit: pressing ENTER confirms and saves, ESC exits without saving
