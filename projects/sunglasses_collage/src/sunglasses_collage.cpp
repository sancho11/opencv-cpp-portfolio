/**
 * @file composite_mars_scene.cpp
 * @brief Overlays multiple objects (sunglasses, mustache, starship, hat, and Elon Musk) onto a Mars
 * background.
 *
 */

#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>

#include "config.pch"

std::string datadir = std::string(DATA_DIR);

/**
 * @brief Loads an image from disk and checks for errors.
 * @param path File path
 * @param flags Imread flags (default: IMREAD_UNCHANGED)
 * @return Loaded cv::Mat
 */
/**
 * @brief Loads an image from disk, exiting on failure.
 * @param path   Full filesystem path to the image.
 * @param flags  OpenCV imread flags (default: IMREAD_UNCHANGED).
 * @return       Loaded cv::Mat.
 */
cv::Mat loadImageOrExit(const std::filesystem::path& path, int flags = cv::IMREAD_UNCHANGED) {
  cv::Mat img = cv::imread(path.string(), flags);
  if (img.empty()) {
    std::cerr << "ERROR: Could not load image: " << path << std::endl;
    std::exit(EXIT_FAILURE);
  }
  return img;
}

/**
 * @brief Saves an image to disk, exiting on failure.
 * @param path    Full filesystem path for output.
 * @param image   Image to write.
 * @param params  Optional imwrite parameters.
 */
void saveImageOrExit(const std::filesystem::path& path, const cv::Mat& image,
                     const std::vector<int>& params = {}) {
  if (!cv::imwrite(path.string(), image, params)) {
    std::cerr << "ERROR: Could not save image: " << path << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

int main() {
  // Window
  const std::string W = "Mars Composite";
  cv::namedWindow(W, cv::WINDOW_NORMAL);
  cv::resizeWindow(W, 1200, 600);

  // Load images
  cv::Mat marsBRG = loadImageOrExit(datadir + "/../data/mars.webp");
  cv::Mat starshipBRG = loadImageOrExit(datadir + "/../data/starship.jpg");
  cv::Mat glassBGR = loadImageOrExit(datadir + "/../data/sunglassRGB.png");
  cv::Mat musktachesBRG = loadImageOrExit(datadir + "/../data/musktache.jpg");
  cv::Mat elonBGR = loadImageOrExit(datadir + "/../data/musk.jpg");
  cv::Mat hatBRG = loadImageOrExit(datadir + "/../data/hat.webp");

  // Using the alpha have some problems  for example if you want to make the eyes to be visible
  // through lenses the non crystal parts will be negatively affected, lets create a 2 mask for
  // lenses 1 for the cristals and one for the whole thing.
  cv::Mat frame_lenses_mask = cv::Mat::zeros(glassBGR.size(), glassBGR.type());
  cv::Mat whole_lenses_mask = cv::Mat::zeros(glassBGR.size(), glassBGR.type());

  // Lets develepop eachone
  cv::inRange(glassBGR, cv::Scalar(0, 0, 0), cv::Scalar(254, 254, 254), whole_lenses_mask);
  cv::inRange(glassBGR, cv::Scalar(0, 0, 55), cv::Scalar(255, 255, 254), frame_lenses_mask);

  // Now lets work on making the mars masks we will need 1 masks in order to have the ground and the
  // sky. cv::Mat marsBRG = cv::imread("../results/mars.webp",cv::IMREAD_UNCHANGED);
  cv::Mat mars_sky_mask = cv::Mat::zeros(marsBRG.size(), marsBRG.type());
  cv::inRange(marsBRG, cv::Scalar(54, 54, 54), cv::Scalar(240, 240, 210), mars_sky_mask);

  // Now lets select the mustache
  // cv::Mat musktachesBRG = imread("../results/musktache.jpg",cv::IMREAD_UNCHANGED);
  cv::Mat selected_musktacheBGR = musktachesBRG(cv::Range(360, 450), cv::Range(370, 580));
  cv::Mat musktache_mask =
      cv::Mat::zeros(selected_musktacheBGR.size(), selected_musktacheBGR.type());
  cv::inRange(selected_musktacheBGR, cv::Scalar(0, 0, 0), cv::Scalar(200, 200, 200),
              musktache_mask);

  // Now lets the hat
  // cv::Mat hatBRG = cv::imread("../results/hat.webp",cv::IMREAD_UNCHANGED);
  cv::Mat hat_mask = cv::Mat::zeros(hatBRG.size(), hatBRG.type());
  cv::inRange(hatBRG, cv::Scalar(0, 0, 0), cv::Scalar(250, 100, 150), hat_mask);

  // Putting it all together
  cv::Mat FinalImage;
  cv::Mat FinalImageChannels[3];

  cv::Mat MarsBRG_Channels[3];
  cv::split(marsBRG, MarsBRG_Channels);
  // Mars Sky First
  for (int i = 0; i < 3; i++) {
    FinalImageChannels[i] = MarsBRG_Channels[i];
  }
  cv::merge(FinalImageChannels, 3, FinalImage);

  // Then Starship
  // We need to resize the starship image to be a quarter of Mars
  cv::Mat starshipBRG_scaleddown;
  double scaleDown = 0.22;
  cv::resize(starshipBRG, starshipBRG_scaleddown, cv::Size(), scaleDown, scaleDown,
             cv::INTER_LINEAR);
  cv::Mat starship_mask1 =
      cv::Mat::zeros(starshipBRG_scaleddown.size(), starshipBRG_scaleddown.type());
  cv::Mat starship_mask2 =
      cv::Mat::zeros(starshipBRG_scaleddown.size(), starshipBRG_scaleddown.type());
  cv::inRange(starshipBRG_scaleddown, cv::Scalar(0, 0, 0), cv::Scalar(255, 80, 255),
              starship_mask1);
  cv::inRange(starshipBRG_scaleddown, cv::Scalar(0, 139, 139), cv::Scalar(255, 255, 255),
              starship_mask2);
  cv::bitwise_or(starship_mask1, starship_mask2, starship_mask1);

  // Elon
  // cv::Mat elonBGR = imread("../data/images/musk.jpg", cv::IMREAD_UNCHANGED);
  cv::Mat elon_mask1 = cv::Mat::zeros(elonBGR.size(), elonBGR.type());
  cv::Mat elon_mask2 = cv::Mat::zeros(elonBGR.size(), elonBGR.type());
  cv::Mat elon_mask3 = cv::Mat::zeros(elonBGR.size(), elonBGR.type());
  cv::inRange(elonBGR, cv::Scalar(110, 0, 9), cv::Scalar(230, 255, 160), elon_mask1);
  cv::inRange(elonBGR, cv::Scalar(0, 0, 9), cv::Scalar(255, 255, 255), elon_mask2);
  cv::inRange(elonBGR, cv::Scalar(0, 0, 0), cv::Scalar(60, 60, 60), elon_mask3);
  elon_mask3(cv::Range(0, elon_mask3.size().height / 2), cv::Range(0, elon_mask3.size().width))
      .setTo(0);

  cv::Mat elon_mask = elon_mask2 - elon_mask1 + elon_mask3;

  cv::Mat elonBGR_Channels[3];
  cv::split(elonBGR, elonBGR_Channels);

  for (int i = 0; i < 3; i++) {
    cv::multiply(elonBGR_Channels[i], elon_mask, elonBGR_Channels[i], 1.0 / 255.0);
  }
  cv::merge(elonBGR_Channels, 3, elonBGR);

  int Ssd_Height = starshipBRG_scaleddown.size().height;
  int Ssd_Width = starshipBRG_scaleddown.size().width;

  cv::Mat starship_working_region =
      (marsBRG(cv::Range(50, 50 + Ssd_Height), cv::Range(240, 240 + Ssd_Width))).clone();
  cv::Mat starship_working_region_channels[3];
  cv::Mat starshipBRG_scaleddown_channels[3];

  cv::split(starship_working_region, starship_working_region_channels);
  cv::split(starshipBRG_scaleddown, starshipBRG_scaleddown_channels);

  for (int i = 0; i < 3; i++) {
    cv::multiply(starshipBRG_scaleddown_channels[i], starship_mask1,
                 starshipBRG_scaleddown_channels[i], 1.0 / 255.0);
    cv::multiply(starship_working_region_channels[i], (255 - starship_mask1),
                 starship_working_region_channels[i], 1.0 / 255.0);
    cv::add(starship_working_region_channels[i], starshipBRG_scaleddown_channels[i],
            starship_working_region_channels[i]);
  }
  cv::merge(starship_working_region_channels, 3, starship_working_region);
  starship_working_region.copyTo(
      FinalImage(cv::Range(50, 50 + Ssd_Height), cv::Range(240, 240 + Ssd_Width)));

  // Now lets apply the martian land
  for (int i = 0; i < 3; i++) {
    FinalImageChannels[i].release();
    MarsBRG_Channels[i].release();
  }
  cv::split(FinalImage, FinalImageChannels);

  // Mars Sky First
  cv::split(marsBRG, MarsBRG_Channels);

  cv::Mat masked_mars_sky_channels[3];
  cv::Mat masked_mars_sky;

  cv::Mat masked_land_channels[3];
  cv::Mat masked_land;

  for (int i = 0; i < 3; i++) {
    cv::multiply(MarsBRG_Channels[i], (1 - mars_sky_mask), masked_mars_sky_channels[i]);
    cv::multiply(FinalImageChannels[i], mars_sky_mask, masked_land_channels[i], 1.0 / 255.0);
    cv::add(masked_land_channels[i], masked_mars_sky_channels[i], FinalImageChannels[i]);
  }
  cv::merge(FinalImageChannels, 3, FinalImage);

  // We have background ready.
  // Lets create our character.
  // Now lets put the glasses, the had and the mustache on.
  cv::Mat eyes_work_areaBGR = elonBGR(cv::Range(150, 150 + glassBGR.size().height),
                                      cv::Range(140, 140 + glassBGR.size().width));
  cv::Mat eyes_work_areaBGR_Channels[3];
  cv::split(eyes_work_areaBGR, eyes_work_areaBGR_Channels);

  // Lets: 1. Apply the mask to the eyes.
  //  2. Put the glasess over the eyes.
  //  3. Get a copy of the frame of the glasses and apply it again over the eyes
  double eyes_transparency = 0.25;
  cv::Mat ROIeyes[3];
  cv::Mat glassBGR_Channels[3];
  cv::Mat frameBGR_Channels[3];
  cv::split(glassBGR, glassBGR_Channels);
  for (int i = 0; i < 3; i++) {
    cv::multiply(eyes_work_areaBGR_Channels[i], 255 - (whole_lenses_mask * (1 - eyes_transparency)),
                 eyes_work_areaBGR_Channels[i], 1.0 / 255.0);
    cv::multiply(glassBGR_Channels[i], (frame_lenses_mask), frameBGR_Channels[i], 1.0 / 255.0);
    cv::multiply(glassBGR_Channels[i], (whole_lenses_mask), glassBGR_Channels[i], 1.0 / 255.0);

    cv::add(eyes_work_areaBGR_Channels[i], glassBGR_Channels[i], eyes_work_areaBGR_Channels[i]);
    cv::multiply(eyes_work_areaBGR_Channels[i], 255 - (frame_lenses_mask),
                 eyes_work_areaBGR_Channels[i], 1.0 / 255.0);
    cv::add(eyes_work_areaBGR_Channels[i], frameBGR_Channels[i], eyes_work_areaBGR_Channels[i]);
  }
  cv::merge(eyes_work_areaBGR_Channels, 3, eyes_work_areaBGR);

  // Now lets go with the mustache
  cv::Mat mustache_work_areaBGR = elonBGR(cv::Range(236, 236 + selected_musktacheBGR.size().height),
                                          cv::Range(185, 185 + selected_musktacheBGR.size().width));
  cv::Mat mustache_work_areaBGR_Channels[3];
  cv::Mat selected_musktacheBGR_Channels[3];
  cv::split(selected_musktacheBGR, selected_musktacheBGR_Channels);
  cv::split(mustache_work_areaBGR, mustache_work_areaBGR_Channels);
  for (int i = 0; i < 3; i++) {
    cv::multiply(mustache_work_areaBGR_Channels[i], 255 - musktache_mask,
                 mustache_work_areaBGR_Channels[i], 1.0 / 255.0);
    cv::multiply(selected_musktacheBGR_Channels[i], musktache_mask,
                 selected_musktacheBGR_Channels[i], 1.0 / 255.0);
    cv::add(selected_musktacheBGR_Channels[i], mustache_work_areaBGR_Channels[i],
            mustache_work_areaBGR_Channels[i]);
  }
  cv::merge(mustache_work_areaBGR_Channels, 3, mustache_work_areaBGR);

  // Lets put Elon on Mars
  double resize_factor = 0.63;
  cv::resize(elonBGR, elonBGR, cv::Size(), resize_factor, resize_factor);
  cv::resize(elon_mask, elon_mask, cv::Size(), resize_factor, resize_factor);
  cv::Mat ElonOnMars_working_area = FinalImage(cv::Range(414, 414 + elonBGR.size().height),
                                               cv::Range(690, 690 + elonBGR.size().width));
  cv::Mat ElonOnMars_working_area_Channels[3];
  cv::split(ElonOnMars_working_area, ElonOnMars_working_area_Channels);
  for (int i = 0; i < 3; i++) {
    cv::multiply(ElonOnMars_working_area_Channels[i], 255 - elon_mask,
                 ElonOnMars_working_area_Channels[i], 1.0 / 255.0);
  }
  cv::merge(ElonOnMars_working_area_Channels, 3, ElonOnMars_working_area);
  ElonOnMars_working_area = ElonOnMars_working_area + elonBGR;

  // Lets add the hat
  resize_factor = 1.4;
  cv::resize(hatBRG, hatBRG, cv::Size(), resize_factor, resize_factor);
  resize(hat_mask, hat_mask, cv::Size(), resize_factor, resize_factor);
  cv::Mat HatOnMars_working_area = FinalImage(cv::Range(300, 300 + hatBRG.size().height),
                                              cv::Range(500, 500 + hatBRG.size().width));
  cv::Mat HatOnMars_working_area_Channels[3];
  cv::Mat hatBRG_Channels[3];
  cv::split(HatOnMars_working_area, HatOnMars_working_area_Channels);
  cv::split(hatBRG, hatBRG_Channels);
  for (int i = 0; i < 3; i++) {
    cv::multiply(HatOnMars_working_area_Channels[i], 255 - hat_mask,
                 HatOnMars_working_area_Channels[i], 1.0 / 255.0);
    cv::multiply(hatBRG_Channels[i], hat_mask, hatBRG_Channels[i], 1.0 / 255.0);
  }
  cv::merge(HatOnMars_working_area_Channels, 3, HatOnMars_working_area);
  cv::merge(hatBRG_Channels, 3, hatBRG);
  HatOnMars_working_area = HatOnMars_working_area + hatBRG;
  cv::imshow(W, FinalImage);
  cv::waitKey(0);

  saveImageOrExit(datadir + "/../data/result.png", FinalImage);
}