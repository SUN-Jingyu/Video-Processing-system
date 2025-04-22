#include "Single_Pixels_head.h"

// Set the brightness offset here
#define BRIGHTNESS_OFFSET   (-50)  // Make image darker

void Brightness(stream_t& stream_in, stream_t& stream_out) {
#pragma HLS INTERFACE axis port=stream_in
#pragma HLS INTERFACE axis port=stream_out
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS PIPELINE II=1


    pixel_t in_pix = stream_in.read();
    pixel_t out_pix;

    // Extract R, G, B components
    uint8_t r = (in_pix.data >> 16) & 0xFF;
    uint8_t g = (in_pix.data >> 8) & 0xFF;
    uint8_t b = (in_pix.data) & 0xFF;

    // Adjust brightness by adding offset
    int r_temp = r + BRIGHTNESS_OFFSET;
    int g_temp = g + BRIGHTNESS_OFFSET;
    int b_temp = b + BRIGHTNESS_OFFSET;

    // Clamp values to [0, 255]
    r = (r_temp > 255) ? 255 : (r_temp < 0) ? 0 : (uint8_t)r_temp;
    g = (g_temp > 255) ? 255 : (g_temp < 0) ? 0 : (uint8_t)g_temp;
    b = (b_temp > 255) ? 255 : (b_temp < 0) ? 0 : (uint8_t)b_temp;

    // Repack R, G, B into 24-bit pixel
    out_pix.data = (r << 16) | (g << 8) | b;

    // Copy control signals!!
    out_pix.keep = in_pix.keep;
    out_pix.strb = in_pix.strb;
    out_pix.user = in_pix.user;  // Frame start
    out_pix.last = in_pix.last;  // End of line
    out_pix.id   = in_pix.id;
    out_pix.dest = in_pix.dest;

    // Output the pixel (no control signals needed)
    stream_out.write(out_pix);
}
