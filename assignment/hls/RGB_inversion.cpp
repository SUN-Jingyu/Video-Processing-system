#include "Single_Pixels_head.h"

void Invert_RGB(stream_t& stream_in, stream_t& stream_out) {
#pragma HLS INTERFACE axis port=stream_in
#pragma HLS INTERFACE axis port=stream_out
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS PIPELINE II=1

    pixel_t in_pix = stream_in.read();
    pixel_t out_pix;

    // Extract R, G, B
    uint8_t r = (in_pix.data >> 16) & 0xFF;
    uint8_t g = (in_pix.data >> 8) & 0xFF;
    uint8_t b = (in_pix.data) & 0xFF;

    // Invert each channel
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;

    // Repack inverted RGB
    out_pix.data = (r << 16) | (g << 8) | b;

    // Copy control signals!!
    out_pix.keep = in_pix.keep;
    out_pix.strb = in_pix.strb;
    out_pix.user = in_pix.user;  // Frame start
    out_pix.last = in_pix.last;  // End of line
    out_pix.id   = in_pix.id;
    out_pix.dest = in_pix.dest;

    // Output the inverted pixel
    stream_out.write(out_pix);
}
