#include "threshold.h"
#include <stdio.h>
using namespace hls;

/*define the thresholds*/


/* gets the grayscale values, returns a 32 bit value with y and x gradients, each occupying 16 bits*/
void hysteresis(axi_stream_8 &inStream, axi_stream_8 &outStream, unsigned char high,unsigned char low) {

#pragma HLS INTERFACE axis port=inStream
#pragma HLS INTERFACE axis port=outStream
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE s_axilite port=high bundle=control
#pragma HLS INTERFACE s_axilite port=low bundle=control

	LineBuffer<WINDOW_DIM,IMG_WIDTH,unsigned char > lineBuff;
	Window<WINDOW_DIM,WINDOW_DIM,unsigned char > window;

	int idxCol=0;
	int idxRow=0;
	int pixConvolved =0;
	unsigned char valin, valout, side;
	uint_8_side_channel dataOutSideChannel;
	uint_8_side_channel currPixelSideChannel;



// LOOP OPTIMIZED 1 -----------------------------
	int flag = 0;
	int npix = 0;

/*first line*/
	for(int w = 0; w< IMG_WIDTH; w++){
//#pragma HLS unroll factor = 100
		lineBuff.shift_up(w);
		if(flag == 0){
			currPixelSideChannel = inStream.read();
			lineBuff.insert_top((unsigned char ) currPixelSideChannel.data, w);
			flag = 1;
		} else {
			lineBuff.insert_top((unsigned char ) inStream.read().data, w);
		}

		dataOutSideChannel.data = 0;
		dataOutSideChannel.keep = currPixelSideChannel.keep;
		dataOutSideChannel.strb = currPixelSideChannel.strb;
		dataOutSideChannel.user = currPixelSideChannel.user;
		dataOutSideChannel.last = 0;
		dataOutSideChannel.id = currPixelSideChannel.id;
		dataOutSideChannel.dest = currPixelSideChannel.dest;

		outStream.write(dataOutSideChannel);
	}
/*other lines*/

	for(int i = 0; i< IMG_HEIGHT -1; i++){
		int pix = 0;
		for(int w = 0; w< IMG_WIDTH; w++){
//#pragma HLS unroll factor =50
#pragma HLS pipeline
			lineBuff.shift_up(w);
			lineBuff.insert_top((unsigned char ) inStream.read().data, w);
		}
		for(pix = 0; pix  < IMG_WIDTH; pix++){
//#pragma HLS unroll factor =50
#pragma HLS pipeline
			window.shift_left();
			for(int wrow = 0; wrow < 3; wrow++){
					unsigned char val = (unsigned char )lineBuff.getval(wrow,pixConvolved);
					window.insert(val,wrow,2);
			}




			if (i > 0 && pix >= 2) {
				valin =(window.getval(1,1));
				valout =0;
				//if less than threshold, suppress
				if (valin < low) {
					valout = 0;
				//if more than threshold, keep
				} else if (valin > high){
					valout = valin;
				//if in between but there are sure neighbours, keep, else suppress
				}
				//if (valin >= low && valin <= high){
#pragma hls pipeline
				else {
					for (int wr = 0; wr<2; wr++) {
						for (int wc = 0; wc<2; wc++){
							side =window.getval(wr,wc);
							if (!(wc == 1 && wr == 1) && side > high) {
								valout = valin;
								wc = wr = 3;
							}
						}
					}
				}

				pixConvolved++;
			}
			if(pixConvolved == 0 || pixConvolved == IMG_WIDTH -2){
				pixConvolved = 0;
			}
				dataOutSideChannel.data = valout;
				dataOutSideChannel.keep = currPixelSideChannel.keep;
				dataOutSideChannel.strb = currPixelSideChannel.strb;
				dataOutSideChannel.user = currPixelSideChannel.user;
				dataOutSideChannel.last = (i+1)  == IMG_HEIGHT -1 && pix  == IMG_WIDTH -1  ? 1 : 0;
				dataOutSideChannel.id = currPixelSideChannel.id;
				dataOutSideChannel.dest = currPixelSideChannel.dest;
					// Send to the stream (Block if the FIFO receiver is full)
				outStream.write(dataOutSideChannel);
		}

	}
}


