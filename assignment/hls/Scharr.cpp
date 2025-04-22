#include "Multi_Pixels_head.h"


void scharr(stream_t& stream_in, stream_t& stream_out) {
#pragma HLS INTERFACE axis port=stream_in
#pragma HLS INTERFACE axis port=stream_out
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS DATAFLOW

    const int rows = MAX_HEIGHT;
    const int cols = MAX_WIDTH;

    // Define intermediate matrices
    xf::cv::Mat<INPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1> img_in(rows, cols);
    xf::cv::Mat<OUTPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1> img_gray(rows, cols);
    xf::cv::Mat<OUTPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1> grad_x(rows, cols);
    xf::cv::Mat<OUTPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1> grad_y(rows, cols);
    xf::cv::Mat<OUTPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1> img_edge(rows, cols);
    xf::cv::Mat<INPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1> img_out(rows, cols); // For RGB output

    // Convert AXI Stream to Mat
    xf::cv::AXIvideo2xfMat(stream_in, img_in);

    // RGB to Grayscale
    xf::cv::rgb2gray<INPUT_TYPE, OUTPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1>(img_in, img_gray);

    // Apply Scharr operator for X and Y gradients
    xf::cv::Scharr<XF_BORDER_CONSTANT, OUTPUT_TYPE, OUTPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1>(img_gray, grad_x, grad_y);

    // Approximate gradient magnitude: abs(grad_x) + abs(grad_y)
    xf::cv::magnitude<XF_L2NORM, OUTPUT_TYPE, OUTPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1>(grad_x, grad_y, img_edge);

    // Gray to RGB (optional, if you want colored output)
    xf::cv::gray2rgb<OUTPUT_TYPE, INPUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPC1>(img_edge, img_out);

    // Mat to AXI Stream
    xf::cv::xfMat2AXIvideo(img_out, stream_out);
}
