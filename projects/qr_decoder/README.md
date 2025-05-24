# QR Code Detector and Annotator

This small tutorial project shows how to take a static image, find any QR code inside it, draw a neat green box around it, display the result in a resizable window, print the decoded text on the console, and finally save the annotated image back to disk. It’s meant as a hands-on example of OpenCV’s built-in QR code tools, basic drawing routines, and simple GUI/file I/O.

---

## 🔑 Key Concepts

- **Modular Image I/O**  
  Encapsulating `cv::imread` and `cv::imwrite` in `loadImage()`/`saveImage()` functions teaches how to centralize your error handling, keep your `main()` clean, and swap in new file-loading logic (e.g. video frames, camera capture) later.

- **QR Code Detection & Decoding**  
  The heart of the example is `cv::QRCodeDetector`. Its `detectAndDecode()` method returns both the decoded string and a vector of four corner points. You’ll learn how OpenCV merges detection and decoding in one step, versus using `detect()` + `decode()` separately.

- **Drawing Primitives**  
  Once you have the four corners, you iterate with `cv::line()` to draw an anti-aliased (`LINE_AA`) bounding polygon. This shows how to annotate any feature you detect—faces, shapes, barcodes, etc.

- **Basic GUI with HighGUI**  
  You’ll see how `namedWindow(..., WINDOW_NORMAL)` plus `resizeWindow()` gives you a flexible display window. `imshow()` and `waitKey()` form the minimal event loop you need for any interactive demo.

- **Configurable Data Directory**  
  By using a compile-time `DATA_DIR` macro (in `config.pch`), you keep all file paths relative and configurable. This pattern scales when you move from one machine to another or package your code.

---

## 📝 Walkthrough of the Core Steps

1. **Load the Image**  
   ```cpp
   cv::Mat img = loadImage("IDCard.jpg");
   ```  
   - Uses `cv::IMREAD_UNCHANGED` by default  
   - Exits if the file is missing or cannot be read

2. **Detect & Decode**  
   ```cpp
   cv::QRCodeDetector qrDecoder;
   std::string text = qrDecoder.detectAndDecode(img, corners);
   ```  
   - `corners` is a `std::vector<cv::Point>` of size 4  
   - If `text.empty()`, no QR code was found

3. **Draw the Bounding Box**  
   ```cpp
   cv::Mat annotated = img.clone();
   for (int i = 0, prev = 3; i < 4; ++i) {
     cv::line(annotated, corners[prev], corners[i],
              cv::Scalar(0,255,0), 2, cv::LINE_AA);
     prev = i;
   }
   ```  
   - Loops through the four corner points in order  
   - Uses a cloned image so the original stays untouched

4. **Display the Result**  
   ```cpp
   cv::namedWindow("QR Result", cv::WINDOW_NORMAL);
   cv::resizeWindow("QR Result", 1200, 600);
   cv::imshow("QR Result", annotated);
   cv::waitKey(0);
   ```  
   - Creates a resizable window  
   - Blocks until a key is pressed

5. **Print & Save**  
   ```cpp
   std::cout << "Decoded Data: " << text << std::endl;
   saveImage("QRCodeAnnotated.jpg", annotated);
   ```  
   - Outputs the decoded string for logging or downstream logic  
   - Saves the annotated image under your project’s `data/` folder

---

## 🚀 For Learners & Reuse

- **Multiple QR Codes**: switch to `detectAndDecodeMulti()` to handle more than one code per image.  
- **Video Streams**: replace `loadImage()` with `cv::VideoCapture` calls and loop over frames.  
- **Performance**: measure `detectAndDecode()` latency on HD images; consider resizing or ROI cropping before detection.  
- **Error-Resilient GUIs**: add keyboard controls (ESC to quit, S to save) so your demo can run unattended.

---

By studying this example you’ll gain a solid grasp of OpenCV’s object-detection pipeline, simple user interfaces, and how to structure small vision utilities for both learning and real-world reuse.