#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/tracking/tracking_legacy.hpp>
#include <iostream>
#include <vector>
#include <filesystem>
#include "config.pch"

// Callback when any trackbar changes
/**
 * @brief Generic trackbar callback to update an integer parameter.
 */
void onTrackbar(int value, void* userdata) { *static_cast<int*>(userdata) = value; }


// Window names
const char* kOptionsWindow = "Options";
const char* kInputWindow   = "Input";

//Global Parameters
static const std::filesystem::path kDataDir = DATA_DIR;
static const std::string kFaceModel = "models/haarcascade_frontalface_default.xml";
static const std::string kEyeModel  = "models/haarcascade_eye.xml";
const int K_FRAMES_DETECTION  = 20;        // cada 30 frames forzamos una redetección
int frameCount = 0;


// Lists of options
std::vector<std::string> input_sources    = {"Webcam", "musk.jpg", "face1.png", "face2.png", "face3.png", "face4.png",};
std::string              glasses          = "sunglassRGB.png";
std::vector<std::string> glasses_images   = {"none", "glasses1.png", "glasses2.png", "glasses5.png", "glasses4.png",
                                            "glasses3.png", "glasses6.png", "glasses7.png",
                                            };
std::vector<std::string> effects_images   = {"none", "effect1.png","effect2.png","effect3.png","effect4.png"};
std::vector<std::string> mustache_images  = {"none", "mustache1.jpg", "mustache2.jpg","mustache2.jpg",
                                            "mustache3.jpg","mustache4.jpg","mustache5.jpg","mustache6.jpg",
                                            "mustache7.jpg","mustache8.jpg","mustache9.jpg","mustache10.jpg",
                                            "mustache11.jpg","mustache12.jpg","mustache13.jpg","mustache14.jpg",
                                            };

// Global parameters updated by trackbars
int g_srcIdx              = 0;  // 0 = Webcam, 1 = input.jpg
int g_glassesImgIdx       = 0;  // index into glasses_images
int g_reflectionContrast  = 0; // 0-100
int g_glassesAlpha        = 0; // 0-100
int g_mustacheOption      = 0;  // index into mustache_images
int g_effectImgIdx        = 0;
int g_effectIntensity     = 0;

/**
 * @brief Displays an image in a resizable window and waits for a key press.
 *
 * Opens a window titled `win`, resizes it to 800×600, shows `img`,
 * then blocks until any key is pressed before destroying the window.
 *
 * @param win  Title of the display window.
 * @param img  Image to display.
 */
static void show(const std::string& win, const cv::Mat& img) {
  cv::namedWindow(win, cv::WINDOW_NORMAL);
  cv::resizeWindow(win, 800, 600);
  cv::imshow(win, img);
  cv::waitKey(0);
  cv::destroyWindow(win);
}

/**
 * detectFacesAndEyesMulti
 *
 * Detecta múltiples caras en la imagen y, para cada cara, detecta ojos
 * dentro de la región de esa cara. Agrupa los ojos por cada cara detectada.
 *
 * @param faceC  (in)  Clasificador Haar Cascade para detección de caras.
 * @param eyeC   (in)  Clasificador Haar Cascade para detección de ojos.
 * @param frame  (in)  Imagen BGR en la que se harán las detecciones.
 * @return Un vector de DetectionResult; cada elemento contiene una
 *         cara detectada y el conjunto de ojos detectados dentro de esa cara.
 */
std::vector<cv::Rect> detectFacesMulti(
    cv::CascadeClassifier& faceC,
    const cv::Mat& frame
) {
    // 1) Convertir a escala de grises y ecualizar histograma (mejorar contraste)
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(gray, gray);

    // 2) Detectar todas las caras posibles en la imagen completa
    std::vector<cv::Rect> faces;
    faceC.detectMultiScale(
        gray,
        faces,
        1.1,                // scaleFactor
        3,                  // minNeighbors
        0 | cv::CASCADE_SCALE_IMAGE,
        cv::Size(100, 100)    // tamaño mínimo de cara
    );

    return faces;
}


/**
 * decideWhetherToRedetect
 *
 * Based on:
 *   - A fixed frame interval (K_FRAMES),
 *   - Any tracker that has low “quality” or has moved out of bounds,
 *   - Whether trackers are already initialized or not.
 *
 * Inputs:
 *   frameCount         – how many frames have passed since last full detection
 *   K_FRAMES_DETECTION – interval to force re-detection
 *   trackersInited     – whether trackers have been created at least once
 *   multiTracker       – the MultiTracker object
 *
 * Returns true if we should drop back to a full Haar detection this frame.
 */
bool decideWhetherToRedetect(int frameCount,
                             int K_FRAMES_DETECTION,
                             bool trackersInited,
                             cv::Ptr<cv::legacy::MultiTracker>& multiTracker,
                             const cv::Mat& frame)
{
    // Always do a detection if we never initialized trackers.
    if (!trackersInited) return true;

    // If we’ve reached the interval, force a detection.
    if (frameCount >= K_FRAMES_DETECTION) {
        return true;
    }

    const std::vector<cv::Rect2d>& vec2d = multiTracker->getObjects();

    std::vector<cv::Rect> trackedBoxes;
    trackedBoxes.reserve(vec2d.size());

    for (const auto& r2d : vec2d) {
        trackedBoxes.emplace_back(r2d);
    }

    // If any tracker has moved entirely out of the frame, we need a re-detection.
    for (size_t i = 0; i < trackedBoxes.size(); ++i) {
        cv::Rect box = trackedBoxes[i];
        cv::Rect imageBounds(0, 0, frame.cols, frame.rows);
        if ((box & imageBounds).area() == 0) {
            // Tracker is completely outside visible area
            return true;
        }
    }

    // If none of the above triggered, continue using existing trackers.
    return false;
}


/**
 * initializeTrackersFromDetections
 *
 * Clears any existing trackers and then adds one KCF tracker for each detected face.
 * Also generates a random color for each tracker so it can be drawn later.
 *
 * Inputs:
 *   detList       – vector of DetectionResult (face + eyes)
 *   frame         – the current frame (needed to initialize the tracker on the right image)
 *   multiTracker  – the MultiTracker instance (will be cleared and re-populated)
 *   trackerColors – vector<Scalar> parallel to multiTracker; cleared and repopulated here
 */
void initializeTrackersFromDetections(const std::vector<cv::Rect>& detList,
                                      const cv::Mat& frame,
                                      cv::Ptr<cv::legacy::MultiTracker>& multiTracker)
{
    // 1) Re-crear el MultiTracker completamente
    multiTracker = cv::legacy::MultiTracker::create();

    // 2) For each detected face, create a new KCF tracker and add it
    for (const auto& det : detList) {
        // Create a new KCF tracker (you could also choose CSRT, MIL, etc.)
        //cv::Ptr<cv::legacy::Tracker> tracker = cv::legacy::TrackerCSRT::create();
        cv::Ptr<cv::legacy::Tracker> tracker = cv::legacy::TrackerMedianFlow::create();
        multiTracker->add(tracker,frame,det);
    }
}


/**
 * updateTrackersOnly
 *
 * Simply updates all trackers in the MultiTracker with the new frame,
 * then draws bounding boxes on the frame using the colors in trackerColors.
 *
 * Inputs:
 *   frame         – current BGR frame (will be modified to draw rectangles)
 *   multiTracker  – the MultiTracker instance with trackers already added
 */
void updateTrackersOnly(cv::Mat& frame,
                        cv::Ptr<cv::legacy::MultiTracker>& multiTracker)
{
    // Update all trackers; they will run their internal predict & correct on this frame
    multiTracker->update(frame);
}

void applyGlasses(
  cv::Mat &frame,
  const cv::Mat &glassesIMG,
  const std::vector<cv::Mat> &glasses_mats,
  int g_glassesImgIdx,
  int g_reflectionContrast,
  int g_glassesAlpha,
  std::vector<cv::Rect> detList,
  const std::vector<cv::Mat> &effects_mats,
  int g_effectImgIdx,
  int g_effectIntensity
) {
  // 1) Generar máscaras base (CV_8UC1) a partir de glassesIMG.
  cv::Mat frame_lenses_mask = cv::Mat::zeros(glassesIMG.size(), CV_8UC1);
  cv::Mat whole_lenses_mask = cv::Mat::zeros(glassesIMG.size(), CV_8UC1);

  // 1a) whole_lenses_mask = píxeles != (0,0,0)
  cv::inRange(glassesIMG,
              cv::Scalar(0, 0, 0),
              cv::Scalar(254, 254, 254),
              whole_lenses_mask);

  // 1b) frame_lenses_mask = región de montura (parte no transparente de los lentes)
  cv::inRange(glassesIMG,
              cv::Scalar(0, 0, 55),
              cv::Scalar(255, 255, 254),
              frame_lenses_mask);

  // 2) Obtener la imagen de reflexión y aplicarle contraste.
  cv::Mat reflection = glasses_mats[g_glassesImgIdx];
  double alpha_contrast = 0.5 + (g_reflectionContrast / 100.0) * 2.0; // [1.0 .. 3.0]
  double betha_contrast =  - (g_reflectionContrast / 100.0) * 128;
  cv::Mat reflection_contrast;
  reflection.convertTo(reflection_contrast, -1, alpha_contrast, betha_contrast);
  // NOTA: Ya no concatenamos aquí. Directamente escalaremos reflection_contrast más adelante.  

  for (size_t i = 0; i < detList.size(); ++i) {
      cv::Rect faceBox = detList[i];
      int faceWidth  = static_cast<int>(faceBox.width);
      int faceHeight = static_cast<int>(faceBox.height);

      // 4) Calcular tamaño final de las gafas (manteniendo proporción original).
      double glassesAspect = static_cast<double>(glassesIMG.cols) / glassesIMG.rows;
      int glassesOutW = faceWidth;
      int glassesOutH = static_cast<int>(faceWidth / glassesAspect);

      // 5) Redimensionar glassesIMG y sus máscaras originales al tamaño final.
      cv::Mat glassesResized, maskFrameResized, maskWholeResized;
      cv::resize(glassesIMG,        glassesResized,    cv::Size(glassesOutW, glassesOutH), 0, 0, cv::INTER_LINEAR);
      cv::resize(frame_lenses_mask,  maskFrameResized, cv::Size(glassesOutW, glassesOutH), 0, 0, cv::INTER_NEAREST);
      cv::resize(whole_lenses_mask,  maskWholeResized, cv::Size(glassesOutW, glassesOutH), 0, 0, cv::INTER_NEAREST);

      // 6) Calcular máscara de lente (“vidrio”): maskLens = whole - frame
      cv::Mat maskLensResized;
      cv::subtract(maskWholeResized, maskFrameResized, maskLensResized);
      //   Ahora:
      //     maskFrameResized(y,x)==255 → montura
      //     maskLensResized(y,x)==255  → lente
      //     resto = 0

      // 7) Generar reflectionTexture escalando reflection_contrast a (glassesOutW × glassesOutH).
      cv::Mat concat_reflection;
      cv::Mat left_ref,right_ref;
      left_ref = reflection_contrast(cv::Range::all(), cv::Range(0,reflection_contrast.cols*0.75));
      right_ref = reflection_contrast(cv::Range::all(), cv::Range(reflection_contrast.cols*0.25,reflection_contrast.cols));

      cv::hconcat(left_ref,right_ref,concat_reflection);


      cv::Mat reflectionTexture;
      cv::resize(concat_reflection,
                 reflectionTexture,
                 cv::Size(glassesOutW, glassesOutH),
                 0, 0, cv::INTER_LINEAR);
      //    reflectionTexture es BGR, tamaño = (glassesOutH × glassesOutW).

      //    - Clonar y usar copyTo() para “pegar” reflectionTexture donde maskLensResized==255.
      cv::Mat gafasConReflection = glassesResized.clone();
      reflectionTexture.copyTo(gafasConReflection, maskLensResized);
      //    En la zona de lente ahora está la textura de reflexión; el resto sigue siendo color original.

      // 9) Construir el canal alpha final (CV_8UC1):
      //    - Para montura (maskFrameResized==255): alpha = 255 * (g_glassesAlpha/100).
      //    - Para lente (maskLensResized==255): alpha = 255.
      //    - Fuera de gafas: alpha = 0.
      cv::Mat newAlpha(glassesOutH, glassesOutW, CV_8UC1, cv::Scalar(0));
      uchar alphaFrameVal = static_cast<uchar>(255.0 * g_glassesAlpha / 100.0);
      newAlpha.setTo(alphaFrameVal,           maskLensResized);  // lente
      newAlpha.setTo(255, maskFrameResized); // montura

      // 11) Calcular posición donde insertar las gafas en el frame:
      int x_offset = static_cast<int>(faceBox.x + (faceWidth - glassesOutW) / 2.0);
      int y_offset = static_cast<int>(faceBox.y + faceHeight * 0.41 - glassesOutH / 2.0);
      x_offset = std::max(0, std::min(x_offset, frame.cols - glassesOutW));
      y_offset = std::max(0, std::min(y_offset, frame.rows - glassesOutH));

      // 12) Extraer ROI del frame
      cv::Mat roi = frame(cv::Rect(x_offset, y_offset, glassesOutW, glassesOutH));

      // 12a) Convertir newAlpha a float [0..1] y expandirlo a 3 canales
      cv::Mat alphaF;
      newAlpha.convertTo(alphaF, CV_32FC1, 1.0/255.0);
      cv::Mat alpha3;
      cv::cvtColor(alphaF, alpha3, cv::COLOR_GRAY2BGR);  // ahora alpha3 es CV_32FC3

      // 12b) Convertir gafasConReflection y roi a CV_32FC3 ([0..1])
      cv::Mat gafRGB_f, roi_f;
      gafasConReflection.convertTo(gafRGB_f, CV_32FC3, 1.0/255.0);
      roi.convertTo(roi_f, CV_32FC3, 1.0/255.0);

      // 12c) Mezclar: blended_f = alpha * gafRGB_f + (1 – alpha) * roi_f
      cv::Mat blended_f = gafRGB_f.mul(alpha3) + roi_f.mul(cv::Scalar::all(1.0) - alpha3);

      if (g_effectImgIdx !=0){
        cv::Mat effectImg = effects_mats[g_effectImgIdx];
        //g_effectIntensity to modulate intensity of the effect.
        cv::Mat gray;
        cv::cvtColor(effectImg, gray, cv::COLOR_BGR2GRAY);

        // (3) Sobel en X e Y (CV_32F)
        cv::Mat sobelX, sobelY;
        cv::Sobel(gray, sobelX, CV_32F, 1, 0, 3);
        cv::Sobel(gray, sobelY, CV_32F, 0, 1, 3);

        // (4) Magnitud del gradiente
        cv::Mat magnitude;
        cv::magnitude(sobelX, sobelY, magnitude);  // CV_32F

        cv::Mat magnitudeNorm;
        cv::normalize(magnitude, magnitudeNorm, 0.0, 255.0, cv::NORM_MINMAX, CV_8U);

        /*cv::Mat maskBinary;
        cv::threshold(magnitudeNorm,
                      maskBinary,
                      50,      // umbral, ajustar al gusto
                      255,
                      cv::THRESH_TOZERO);*/

        cv::Mat maskOpenResized;
        cv::resize(magnitudeNorm,maskOpenResized,blended_f.size());
        //show("Morph",maskBinary);

        // (7) Convertir máscara a CV_32F [0..1]
        cv::Mat maskFinal;
        maskOpenResized.convertTo(maskFinal, CV_32F, 1.0 / 255.0);  // CV_32F, [0..1]

        // (8) Convertir blended_f a HSV (float [0..1])
        cv::Mat hsv_f;
        cv::cvtColor(blended_f, hsv_f, cv::COLOR_BGR2HSV); // hsv_f es CV_32FC3: H∈[0..360), S,V∈[0..1]

        // (9) Separar canales H, S, V
        std::vector<cv::Mat> canales;
        cv::split(hsv_f, canales);
        cv::Mat& hChan = canales[0];  // CV_32F
        cv::Mat& sChan = canales[1];  // CV_32F
        cv::Mat& vChan = canales[2];  // CV_32F

        // (10) Definir coeficientes de modificación en zonas de “rayadura”
        float coefS = 1.0f-g_effectIntensity/100;  // reducir saturación al 40%
        float coefV = 20*g_effectIntensity/100;  // aumentar brillo al 120%

        // (11) Calcular S y V modificados:
        //      sMod = sChan*(1 - maskFinal) + (sChan*coefS)*maskFinal
        //      vMod = vChan*(1 - maskFinal) + (vChan*coefV)*maskFinal
        maskFinal=maskFinal.mul(alphaF);
        cv::threshold(maskFinal, maskFinal, 1.0f, 1.0f, cv::THRESH_TRUNC);

        cv::Mat sMod = sChan.mul(1.0f - maskFinal) + sChan.mul(coefS).mul(maskFinal);
        cv::Mat vMod = vChan.mul(1.0f - maskFinal) + vChan.mul(coefV).mul(maskFinal);

        // (12) Clip a [0..1]
        cv::threshold(sMod, sMod, 1.0f, 1.0f, cv::THRESH_TRUNC);
        cv::threshold(vMod, vMod, 1.0f, 1.0f, cv::THRESH_TRUNC);

        // (13) Reemplazar los canales S y V en hsv_f
        sMod.copyTo(canales[1]);
        vMod.copyTo(canales[2]);

        // (14) Reconstruir HSV modificado y convertir de nuevo a BGR float [0..1]
        cv::Mat hsvMod_f;
        cv::merge(canales, hsvMod_f);                       // CV_32FC3
        cv::cvtColor(hsvMod_f, blended_f, cv::COLOR_HSV2BGR); // blended_f vuelve a CV_32FC3
      }

      // 12d) Volver a 8-bit y copiar en el ROI
      blended_f.convertTo(roi, CV_8UC3, 255.0);
  }
}

void applyMustache(cv::Mat &frame,
  const std::vector<cv::Mat> &mustache_mats,
  int g_mustacheImgIdx,
  std::vector<cv::Rect> detList) 
{
  // 1) Obtener la imagen de bigote actual y su máscara
  cv::Mat mustacheRaw = mustache_mats[g_mustacheImgIdx].clone(); // sin canal alpha

  // 2) Crear máscara: todo lo que no sea casi blanco es considerado parte del bigote
  cv::Mat baseMask;
  cv::inRange(mustacheRaw,
  cv::Scalar(0, 0, 0),
  cv::Scalar(100, 100, 100),
  baseMask); // blanco fuera, negro dentro
  //cv::bitwise_not(baseMask, baseMask); // ahora: blanco = bigote, negro = fondo

  // 3) Detectar el punto rojo (~bajo la nariz)
  cv::Mat maskRed;
  cv::inRange(mustacheRaw,
  cv::Scalar(0, 0, 100),
  cv::Scalar(80, 80, 255),
  maskRed);
  //show("maskRed",maskRed);
  cv::Moments m = cv::moments(maskRed, true);
  cv::Point redDot;
  if (m.m00 > 0) {
    redDot = cv::Point(static_cast<int>(m.m10 / m.m00), static_cast<int>(m.m01 / m.m00));
    // eliminar la zona del punto rojo de la máscara
    cv::circle(mustacheRaw, redDot, 2, cv::Scalar(0), -1);
    cv::circle(baseMask, redDot, 2, cv::Scalar(255), -1);
  } else {
    // Si no hay punto rojo, evitar fallo
    redDot = cv::Point(mustacheRaw.cols / 2, mustacheRaw.rows / 2);
  }
  //show("baseMask",baseMask);

  // 4) Para cada detección (rostro)
  for (const auto& faceBox : detList) {
  int faceWidth  = faceBox.width;
  int faceHeight = faceBox.height;

  // 5) Calcular dimensiones del bigote proporcional a la cara
  int mustacheOutW = static_cast<int>(faceWidth * 0.6);
  double aspectRatio = static_cast<double>(mustacheRaw.cols) / mustacheRaw.rows;
  int mustacheOutH = static_cast<int>(mustacheOutW / aspectRatio);

  // 6) Redimensionar imagen del bigote y la máscara
  cv::Mat mustacheResized, maskResized;
  cv::resize(mustacheRaw, mustacheResized, cv::Size(mustacheOutW, mustacheOutH), 0, 0, cv::INTER_LINEAR);
  cv::resize(baseMask, maskResized, cv::Size(mustacheOutW, mustacheOutH), 0, 0, cv::INTER_NEAREST);

  // 7) Calcular nueva posición del punto rojo tras redimensionar
  float scaleX = static_cast<float>(mustacheOutW) / mustacheRaw.cols;
  float scaleY = static_cast<float>(mustacheOutH) / mustacheRaw.rows;
  cv::Point redDotRescaled(static_cast<int>(redDot.x * scaleX), static_cast<int>(redDot.y * scaleY));

  // 8) Calcular coordenadas en el rostro
  int x_offset = faceBox.x + (faceWidth - mustacheOutW) / 2;
  int y_offset = faceBox.y + static_cast<int>(faceHeight * 0.65) - redDotRescaled.y;

  // Asegurar que se mantenga dentro del frame
  x_offset = std::max(0, std::min(x_offset, frame.cols - mustacheOutW));
  y_offset = std::max(0, std::min(y_offset, frame.rows - mustacheOutH));

  // 9) ROI del frame
  cv::Mat roi = frame(cv::Rect(x_offset, y_offset, mustacheOutW, mustacheOutH));

  // 10) Convertir máscara a 3 canales para blending
  cv::Mat mask3c, mustache_f, roi_f, mask_f;
  cv::cvtColor(maskResized, mask3c, cv::COLOR_GRAY2BGR);
  mask3c.convertTo(mask_f, CV_32FC3, 1.0 / 255.0);
  mustacheResized.convertTo(mustache_f, CV_32FC3, 1.0 / 255.0);
  roi.convertTo(roi_f, CV_32FC3, 1.0 / 255.0);

  // 11) Composición: alpha blend simple
  cv::Mat blended = mustache_f.mul(mask_f) + roi_f.mul(cv::Scalar::all(1.0) - mask_f);

  // 12) Convertir a 8 bits y colocar en el ROI original
  blended.convertTo(roi, CV_8UC3, 255.0);
  }
}


int main() {
    // 1. Create windows and trackbars
    cv::namedWindow(kOptionsWindow, cv::WINDOW_NORMAL);
    cv::resizeWindow(kOptionsWindow, 600, 400);
    cv::namedWindow(kInputWindow, cv::WINDOW_NORMAL);
    cv::resizeWindow(kInputWindow, 600, 400);

    cv::createTrackbar("Source",               kOptionsWindow, nullptr,
                        static_cast<int>(input_sources.size()-1),
                        onTrackbar, &g_srcIdx);
    cv::createTrackbar("Glasses Image",        kOptionsWindow, nullptr,
                        static_cast<int>(glasses_images.size()-1),
                        onTrackbar, &g_glassesImgIdx);
    cv::createTrackbar("Reflection Contrast",  kOptionsWindow, nullptr,
                        100, onTrackbar, &g_reflectionContrast);
    cv::createTrackbar("Glasses Alpha",        kOptionsWindow, nullptr,
                        100, onTrackbar, &g_glassesAlpha);
    cv::createTrackbar("Glasses Effect",        kOptionsWindow, nullptr,
                        static_cast<int>(effects_images.size()-1),
                        onTrackbar, &g_effectImgIdx);
    cv::createTrackbar("Effect Intensity",        kOptionsWindow, nullptr,
                          100, onTrackbar, &g_effectIntensity);
    cv::createTrackbar("Mustache Option",      kOptionsWindow, nullptr,
                        static_cast<int>(mustache_images.size()-1),
                        onTrackbar, &g_mustacheOption);

    //Load HarrCascade Models
    cv::CascadeClassifier faceC;
    if (!faceC.load((kDataDir / "../data" / kFaceModel).string())){
        std::cerr << "ERROR: Could not load face cascade.\n";
        return -1;
    }

    // Pre-load static images into memory
    std::vector<cv::Mat> images;
    for (size_t i = 0; i < input_sources.size(); ++i) {
        if (i == 0) {
            images.push_back(cv::Mat());
            continue;
        } else {
            cv::Mat img = cv::imread(kDataDir / "../data" / input_sources[i], cv::IMREAD_COLOR);
            //std::cout << input_sources[i] << std::endl;
            if (img.empty()) {
                std::cerr << "Error: Could not preload image '"
                          << input_sources[i] << "'." << std::endl;
            }
            images.push_back(img);
        }
    }

    // Pre-load glasses images
    cv::Mat glassesIMG=cv::imread(kDataDir / "../data" / glasses, cv::IMREAD_COLOR);
    if (glassesIMG.empty()) {
        std::cerr << "Error: Could not preload glasses image '"
                << glasses << "'." << std::endl;
    }

    std::vector<cv::Mat> glasses_mats;
    for (size_t i = 0; i < glasses_images.size(); ++i) {
        if (i == 0) {
            glasses_mats.push_back(cv::Mat());
            continue;
        } else {
            cv::Mat gimg = cv::imread(kDataDir / "../data" / glasses_images[i], cv::IMREAD_COLOR);
            if (gimg.empty()) {
                std::cerr << "Error: Could not preload glasses image '"
                        << glasses_images[i] << "'." << std::endl;
            }
            glasses_mats.push_back(gimg);
        }
    }

    std::vector<cv::Mat> effects_mats;
    for (size_t i = 0; i < effects_images.size(); ++i) {
        if (i == 0) {
          effects_mats.push_back(cv::Mat());
            continue;
        } else {
            cv::Mat gimg = cv::imread(kDataDir / "../data" / effects_images[i], cv::IMREAD_COLOR);
            if (gimg.empty()) {
                std::cerr << "Error: Could not preload glasses image '"
                        << effects_images[i] << "'." << std::endl;
            }
            effects_mats.push_back(gimg);
        }
    }

    // Pre-load mustache images
    std::vector<cv::Mat> mustache_mats;
    for (size_t i = 0; i < mustache_images.size(); ++i) {
        if (i == 0) {
            mustache_mats.push_back(cv::Mat());
            continue;
        } else {
            cv::Mat mimg = cv::imread(kDataDir / "../data" / mustache_images[i], cv::IMREAD_COLOR);
            if (mimg.empty()) {
                std::cerr << "Error: Could not preload mustache image '"
                          << mustache_images[i] << "'." << std::endl;
            }
            mustache_mats.push_back(mimg);
        }
    }

    // Initialize webcam
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cout << "Warning: Could not open webcam, using static images only." << std::endl;
    }else{
      cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
      cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    }

    std::vector<cv::Rect> detList;
    bool finish_execution=false;
    int prev_src_idx = -1;


    cv::Ptr<cv::legacy::MultiTracker> multiTracker = cv::legacy::MultiTracker::create();

    bool trackersInited = false;       // have we ever added trackers?
    int frameCount     = 0;            // counts frames since last full detection

    // Main loop
    while (true) {
        cv::Mat frame;
        // 2. Read frame based on current source selection
        if (g_srcIdx == 0 && cap.isOpened()) {
            cap >> frame;
            if (frame.empty()) {
                std::cerr << "Error: Empty frame from webcam." << std::endl;
                break;
            }
            ++frameCount;
            bool doRedetection = decideWhetherToRedetect( frameCount, K_FRAMES_DETECTION,trackersInited,
                                                            multiTracker, frame
            );

            if (doRedetection) {
                // 4.1) Perform Haar-based detection
                std::vector<cv::Rect> detList = detectFacesMulti(faceC, frame);
    
                // 4.2) If we found at least one face, re-initialize trackers
                if (!detList.empty()) {
                    initializeTrackersFromDetections(detList, frame, multiTracker);
                    trackersInited = true;
                }
                // Reset frameCount so we count again for the next interval
                frameCount = 0;
            }
            // Update existing trackers
            updateTrackersOnly(frame, multiTracker);
            const std::vector<cv::Rect2d>& vec2d = multiTracker->getObjects();

            // 2) Creas un vector de cv::Rect (int) del mismo tamaño:
            std::vector<cv::Rect> trackedBoxes;
            detList.clear();
            detList.reserve(vec2d.size());

            for (const auto& r2d : vec2d) {
              detList.emplace_back(r2d);
            }
    
          //Glasses Logic
          if (g_glassesImgIdx != 0) {
            applyGlasses(frame,
              glassesIMG,
              glasses_mats,
              g_glassesImgIdx,
              g_reflectionContrast,
              g_glassesAlpha,
              detList,
              effects_mats,
              g_effectImgIdx,
              g_effectIntensity
            );
          }
           //Mustache Logic
           if (g_mustacheOption != 0){
            applyMustache(frame,
              mustache_mats,
              g_mustacheOption,
              detList);
          }
        } else {
            frame = images[g_srcIdx].clone();
            if (frame.empty()) {
                std::cerr << "Error: Preloaded image is empty at index "
                        << g_srcIdx << "." << std::endl;
                break;
            }
            //Do the thing
            detList.clear();
            std::vector<cv::Rect> detList = detectFacesMulti(faceC, frame);
            if (g_glassesImgIdx != 0) {
              applyGlasses(frame,
                glassesIMG,
                glasses_mats,
                g_glassesImgIdx,
                g_reflectionContrast,
                g_glassesAlpha,
                detList,
                effects_mats,
                g_effectImgIdx,
                g_effectIntensity
              );
            }
            if (g_mustacheOption != 0){
              applyMustache(frame,
                mustache_mats,
                g_mustacheOption,
                detList);
            }

            }
            //Do nothing until change
            /*while (!(g_srcIdx == 0 && cap.isOpened())) {
                cv::imshow(kInputWindow, frame);
                int skey = cv::waitKey(30);
                if (skey == 27) {
                    finish_execution = true;
                    break;
                }
            } */

        // Show input
        cv::imshow(kInputWindow, frame);

        // Wait for ESC key (27) for 30ms
        int key = cv::waitKey(30);
        if (key == 27 || finish_execution) {
            break;
        }
    }

    return 0;
}