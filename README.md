# OpenCV C++ Portfolio

Welcome to my **OpenCV C++ Portfolio**, a curated collection of computer vision projects and assignments demonstrating practical experience with OpenCV and C++.

---

## Repository Structure

```
├── CMakeLists.txt           # Top-level build configuration
├── LICENSE                  # MIT License
├── README.md                # This overview and quick start guide
├── .gitignore
├── .clang-format
│
└── projects/                # Self-contained OpenCV demos and assignments.
    ├── sunglasses_collage/  # Fun sunglasses filter collage.
    ├── qr_decoder/          # QR code detection and decoding.
    ├── roi_selector/        # Interactive ROI selection tool.
    ├── interactive_scaler/  # Real-time image scaling with trackbars.
    ├── autofocus_evaluator/ # Frame focus evaluation using variance metrics.
    ├── sketch_and_cartoon/  # Pencil sketch and cartoon effect filters.
    ├── blemish_removal/     # Interactive facial imperfection removal.
    ├── chroma_key/          # Color-based background replacement (chroma key).
    ├── panorama_stitching/  # Panorama creation using OpenCV Stitcher API.
    ├── feature_alignment/   # ORB feature matching and homography alignment.
    └── skin_smoothing/      # Like blemish removal but this have additional improvements for an automatic detection of areas to fix.
```

Each subfolder under `projects/` contains:
- `include/` directory for header files
- `src/` directory for source files
- `data/` for sample images and assets
- Its own `CMakeLists.txt` to build the executable
- `README.md` with specific usage notes and examples

---

## Prerequisites

- **C++17** compatible compiler
- **CMake** version ≥ 3.10
- **OpenCV** library version ≥ 4.11

On Ubuntu-like systems, you can install dependencies with:

```bash
sudo apt update
sudo apt install build-essential cmake libopencv-dev
```

---

## Build & Run Instructions

Clone the repository and build all projects in one step:

```bash
https://github.com/sancho11/opencv-cpp-portfolio.git
cd opencv-cpp-portfolio
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Each executable will be placed in the bin directory (e.g., `build/bin/qr_decoder`).

To run a specific project:

```bash
./bin/<project_name>
```

---

## License

This portfolio is released under the [MIT License](LICENSE).

---

## Contributing

Although this is primarily a personal portfolio, you are welcome to:

1. Fork the repository
2. Submit issues for bug reports or feature requests
3. Open pull requests with improvements or new demos

(Optional: No strict contribution guidelines are required unless you intend to grow this into a community project.)

*Happy coding!*  
*Feel free to explore each demo and adapt the code to your own experiments.*