#include "Multi_Pixels_head.h"

void sobel(stream_t& stream_in, stream_t& stream_out) {
#pragma HLS INTERFACE axis port=stream_in
#pragma HLS INTERFACE axis port=stream_out
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS DATAFLOW

    const int rows = MAX_HEIGHT;
    const int cols = MAX_WIDTH;

    // 1. Intermediate images
    xf::cv::Mat<INPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1> img_in(rows, cols);
    xf::cv::Mat<OUTPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1> img_gray(rows, cols);
    xf::cv::Mat<OUTPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1> grad_x(rows, cols);
    xf::cv::Mat<OUTPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1> grad_y(rows, cols);
    xf::cv::Mat<OUTPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1> img_edge(rows, cols);
    xf::cv::Mat<INPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1> img_out(rows, cols); // <-- RGB output format

    // 2. Stream to Mat
    xf::cv::AXIvideo2xfMat(stream_in, img_in);

    //xf::cv::rgb2bgr<INPUT_TYPE, INPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1>(img_in, img_out);

    // 3. RGB to Grayscale
    xf::cv::rgb2gray<INPUT_TYPE, OUTPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1>(img_in, img_gray);

    // 4. Sobel filter to get X and Y gradients
    xf::cv::Sobel<XF_BORDER_CONSTANT, XF_FILTER_3X3, OUTPUT_TYPE, OUTPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1>(img_gray, grad_x, grad_y);

    // 5. True magnitude calculation: sqrt(grad_x^2 + grad_y^2) using Normalization L2 type, consuming more on-board source
    xf::cv::magnitude<XF_L2NORM, OUTPUT_TYPE, OUTPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1>(grad_x, grad_y, img_edge);

    // 6. Grayscale to RGB
    xf::cv::gray2rgb<OUTPUT_TYPE, INPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1>(img_edge, img_out);

    // 7. Mat to Stream
    xf::cv::xfMat2AXIvideo(img_out, stream_out);
}
