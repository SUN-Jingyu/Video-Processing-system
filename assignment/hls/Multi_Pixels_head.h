
#include "ap_int.h"
#include "hls_stream.h"

#include "common/xf_utility.hpp"
#include "common/xf_infra.hpp"
#include "common/xf_common.hpp"    // For xf::cv::Mat
#include "imgproc/xf_canny.hpp"    // If you use Canny Edge Detection
#include "imgproc/xf_cvt_color.hpp"
#include "imgproc/xf_sobel.hpp"
#include "imgproc/xf_gaussian_filter.hpp"
#include "imgproc/xf_add_weighted.hpp"
#include  <core/xf_arithm.hpp>
#include "core/xf_magnitude.hpp" 	//used for magnitude
#include "imgproc/xf_scharr.hpp"

// AXI Stream interface
typedef ap_axiu<24, 1, 1, 1> interface_t;
typedef hls::stream<interface_t> stream_t;

// Image size
#define MAX_HEIGHT 1080
#define MAX_WIDTH  1920
// Pixel types
#define INPUT_TYPE XF_8UC3  	// Input: RGB image
#define OUTPUT_TYPE XF_8UC1  	// Grayscale or Edge map
#define NPC1 XF_NPPC1      		// 1 pixel per clock
#define NPC4 XF_NPPC4      		// 4 pixel per clock


// Top-level function declaration
void Gaussian_Blur(stream_t& stream_in, stream_t& stream_out);

void sobel(stream_t& stream_in, stream_t& stream_out);

void scharr(stream_t& stream_in, stream_t& stream_out);


// Image paths for simulation (optional)
#define INPUT_IMAGE "fox.bmp"
#define OUTPUT_IMAGE "fox_output.bmp"

