
#include "ap_int.h"
#include "hls_stream.h"

#include "ap_axi_sdata.h"

// AXI Stream interface
typedef ap_axiu<24, 1, 1, 1> pixel_t;
typedef hls::stream<pixel_t> stream_t;


// Top-level function declaration
void Invert_RGB(stream_t& stream_in, stream_t& stream_out);

void RGB2Gray(stream_t& stream_in, stream_t& stream_out);

void Brightness(stream_t& stream_in, stream_t& stream_out);


// Image paths for simulation (optional)
#define INPUT_IMAGE "fox.bmp"
#define OUTPUT_IMAGE "fox_output.bmp"

