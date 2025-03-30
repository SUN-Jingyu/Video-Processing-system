/*
 *  HLS Matrix multiplication accelerator
 *
 *  Updated by: Callum Stew
 *  Adapted from work by: Eduardo Wachter
 *  Date: 23/07/2024
 *
 *  Baud rate: 115200
 */

#include "matrix_mult.h"

void array_mult (stream_data &in_a, data in_b[ROWS*COLS], stream_data &result)
{
    // Define the AXI interfaces
	#pragma HLS INTERFACE s_axilite port=return bundle=CTRL
	#pragma HLS INTERFACE s_axilite port=in_b bundle=DATA_IN_B
	#pragma HLS INTERFACE axis port=in_a
	#pragma HLS INTERFACE axis port=result

	data i,j,k;                   // Loop counters
	packet mult_acc;              // Variable to store the result and accumilation of the multiplications
	packet in_a_store[ROWS*COLS]; // Local memory to store the input matrix

	// Read the input matrix from the stream and store it in the local memory
	for (i=0;i<ROWS*COLS;i++) {
		in_a.read(in_a_store[i]); // Read value form the stream
	}

	ROWS_LOOP: for (i=0;i<ROWS;i++) { // Iterates through rows in in_a
		// #pragma HLS unroll factor=2
		COLS_LOOP: for (j=0;j<COLS;j++) { // Iterates through columns in in_b
			// #pragma HLS unroll
			mult_acc.data=0; // Initialize the accumulator

			MULT_ACC_LOOP: for (k=0;k<MULT_ACC;k++) { // For each value in the row/column
				// #pragma HLS PIPELINE II=2

				mult_acc.data+=in_a_store[i*ROWS+k].data*in_b[k*ROWS+j]; // multiply and accumulate values
				mult_acc.last=in_a_store[i*ROWS+k].last&(j==(COLS-1)); // If it is the last use of the last value in in_a stream, set last to 1
				mult_acc.keep = in_a_store[i*ROWS+k].keep;
				mult_acc.strb = in_a_store[i*ROWS+k].strb;
			}

			result.write(mult_acc); // Write the result to the stream
		}
	}
}
