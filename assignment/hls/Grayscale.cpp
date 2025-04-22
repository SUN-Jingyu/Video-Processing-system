#include "Single_Pixels_head.h"


void RGB2Gray(stream_t& stream_in, stream_t& stream_out) {
#pragma HLS INTERFACE axis port=stream_in
#pragma HLS INTERFACE axis port=stream_out
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS PIPELINE II=1

    pixel_t in_pix = stream_in.read();
    pixel_t out_pix;

    uint8_t r = (in_pix.data >> 16) & 0xFF;
    uint8_t g = (in_pix.data >> 8) & 0xFF;
    uint8_t b = (in_pix.data) & 0xFF;

    // Grayscale formula: 0.299*R + 0.587*G + 0.114*B
    uint8_t gray = (uint8_t)((77 * r + 150 * g + 29 * b) >> 8); // Integer approximation

    // Output pixel: set R = G = B = gray
    out_pix.data = (gray << 16) | (gray << 8) | gray;

    // Copy control signals!!
    out_pix.keep = in_pix.keep;
    out_pix.strb = in_pix.strb;
    out_pix.user = in_pix.user;  // Frame start
    out_pix.last = in_pix.last;  // End of line
    out_pix.id   = in_pix.id;
    out_pix.dest = in_pix.dest;

    stream_out.write(out_pix);
}
