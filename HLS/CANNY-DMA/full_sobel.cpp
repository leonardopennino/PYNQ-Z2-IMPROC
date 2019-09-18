#include "hls_video.h"
#include <ap_axi_sdata.h>

#define IMG_WIDTH 1280
#define IMG_HEIGHT 720
#define KERNEL_DIM 3

typedef ap_axiu<8,1,1,1> uint_8_side_channel;
typedef ap_axiu<32,1,1,1> uint_32_side_channel;

typedef hls::stream<uint_32_side_channel> axi_stream_32;
typedef hls::stream<uint_8_side_channel> axi_stream_8;

short  add(short, short);
using namespace hls;



/* gets the grayscale values, returns a 32 bit value with y and x gradients, each occupying 16 bits*/
void sobel_project(stream<uint_8_side_channel> &inStream, stream<uint_32_side_channel> &outStream) {

#pragma HLS INTERFACE axis port=inStream
#pragma HLS INTERFACE axis port=outStream
#pragma HLS INTERFACE ap_ctrl_none port=return
	LineBuffer<KERNEL_DIM,IMG_WIDTH,unsigned char> lineBuff;
	Window<KERNEL_DIM,KERNEL_DIM,short> window;

	int idxCol=0;
	int idxRow=0;
	int pixConvolved =0;

	uint_32_side_channel dataOutSideChannel;
	uint_8_side_channel currPixelSideChannel;



// LOOP OPTIMIZED 1 -----------------------------
	int flag = 0;
	int npix = 0;

/*first line
 * load lineBuffer, unload zeroes
 * */
	for(int w = 0; w< IMG_WIDTH; w++){
#pragma pipeline
		lineBuff.shift_up(w);
		if(flag == 0){
			currPixelSideChannel = inStream.read();
			lineBuff.insert_top((unsigned char) currPixelSideChannel.data, w);
			flag = 1;
		} else {
			lineBuff.insert_top((unsigned char) inStream.read().data, w);
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
/*other lines, load linebuffer. Unload linebuffer by burst, load window, calculate gradients*/

	for(int i = 0; i< IMG_HEIGHT -1; i++){
		int pix = 0;

		//load line
		for(int w = 0; w< IMG_WIDTH; w++){

#pragma HLS pipeline
			lineBuff.shift_up(w);
			lineBuff.insert_top((unsigned char) inStream.read().data, w);
		}

		//unload line
		for(pix = 0; pix  < IMG_WIDTH; pix++){

#pragma HLS pipeline
			//update window
			window.shift_left();
			for(int wrow = 0; wrow < 3; wrow++){
					short val = (short)lineBuff.getval(wrow,pixConvolved);
					window.insert(val,wrow,2);
			}

			unsigned int g = 0;
			unsigned int valout;
			unsigned short ux,uy;

			//do the convolutions
			if (i > 0 && pix >= 2) {

				short res1_x = add(-window.getval(0,0), window.getval(0,2));
				short res2_x =  2*add(-window.getval(1,0) , window.getval(1,2));
				short res3_x = add(-window.getval(2,0) , window.getval(2,2));
				short sum1_x = add(res1_x,res2_x);

				ux = sum1_x + res3_x;

				short res1_y = add(-window.getval(0,0), window.getval(2,0));
				short res2_y =  2*add(-window.getval(0,1) , window.getval(2,1));
				short res3_y = add(-window.getval(0,2) , window.getval(2,2));
				short sum1_y = add(res1_y,res2_y);

				uy = sum1_y + res3_y;

				g = uy;


				valout = (g << 16) |  ux;

				pixConvolved++;
			}
			//exclude limit pixels

			if(pixConvolved == 0 || pixConvolved == IMG_WIDTH -2){
				pixConvolved = 0;
			}

			//send output to stream

			dataOutSideChannel.data = valout;
			dataOutSideChannel.keep = currPixelSideChannel.keep;
			dataOutSideChannel.strb = currPixelSideChannel.strb;
			dataOutSideChannel.user = currPixelSideChannel.user;
			dataOutSideChannel.last = (i+1)  == IMG_HEIGHT -1 && pix  == IMG_WIDTH -1  ? 1 : 0; //1 if end image - might change to eol
			dataOutSideChannel.id = currPixelSideChannel.id;
			dataOutSideChannel.dest = currPixelSideChannel.dest;

			outStream.write(dataOutSideChannel);
		}

	}
}

short add(short a, short b){
#pragma HLS inline
	return a+b;
}

