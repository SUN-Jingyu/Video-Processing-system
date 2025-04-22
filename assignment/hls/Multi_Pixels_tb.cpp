#include "Multi_Pixels_head.h"
#include "hls_stream.h"
#include "common/xf_axi.hpp"       // For AXI4-Stream types
#include "opencv2/opencv.hpp"       // OpenCV for simulation

// Function declarations (already included in header)
void Gaussian_Blur(stream_t& stream_in, stream_t& stream_out);
void Sobel(stream_t& stream_in, stream_t& stream_out);
void Scharr(stream_t& stream_in, stream_t& stream_out);

int main() {
    const int rows = MAX_HEIGHT;
    const int cols = MAX_WIDTH;

    // Read the input image
    cv::Mat src = cv::imread(INPUT_IMAGE, cv::IMREAD_COLOR);
    if (src.empty()) {
        std::cout << "Error: Cannot read input image: " << INPUT_IMAGE << std::endl;
        return -1;
    }

    // Resize if necessary
    if (src.rows != rows || src.cols != cols) {
        cv::resize(src, src, cv::Size(cols, rows));
    }

    // Declare AXI streams
    stream_t stream_in, stream_gaussian_out, stream_sobel_out, stream_scharr_out;

    // ---- Gaussian Blur ----
    xf::cv::cvMat2AXIvideoxf<NPC1, 24>(src, stream_in);   // reload input
    Gaussian_Blur(stream_in, stream_gaussian_out);

    cv::Mat gaussian_img(rows, cols, CV_8UC3);
    xf::cv::AXIvideo2cvMatxf<NPC1, 24>(stream_gaussian_out, gaussian_img);
    cv::imwrite("gaussian_blur_output.bmp", gaussian_img);

    // ---- Sobel ----
    stream_t stream_in_sobel;
    xf::cv::cvMat2AXIvideoxf<NPC1, 24>(src, stream_in_sobel); // reload input
    sobel(stream_in_sobel, stream_sobel_out);

    cv::Mat sobel_img(rows, cols, CV_8UC3);
    xf::cv::AXIvideo2cvMatxf<NPC1, 24>(stream_sobel_out, sobel_img);
    cv::imwrite("sobel_output.bmp", sobel_img);

    // ---- Scharr ----
    stream_t stream_in_scharr;
    xf::cv::cvMat2AXIvideoxf<NPC1, 24>(src, stream_in_scharr); // reload input
    scharr(stream_in_scharr, stream_scharr_out);

    cv::Mat scharr_img(rows, cols, CV_8UC3);
    xf::cv::AXIvideo2cvMatxf<NPC1, 24>(stream_scharr_out, scharr_img);
    cv::imwrite("scharr_output.bmp", scharr_img);

    std::cout << "All filters applied and images saved successfully!" << std::endl;
    return 0;
}
