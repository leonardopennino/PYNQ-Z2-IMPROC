#include "hls_video.h"
#define IMG_WIDTH 1280
#define IMG_HEIGHT 720
#include <ap_axi_sdata.h>

#define WINDOW_DIM 3
#define PI 3.14159265

typedef ap_axiu<32,1,1,1> uint_32_side_channel;
typedef ap_axiu<8,1,1,1> uint_8_side_channel;

typedef hls::stream<uint_32_side_channel> axi_stream_32;
typedef hls::stream<uint_8_side_channel> axi_stream_8;

using namespace hls;

/*This version uses sqrt and atan instead of approximations.
 * The tradeoff comes with performances in exchange for precision
 */

/* theoretical maximum given root calculated gradient (max = 255*4 [sobel] * sqrt(2) [gradient] =1442.5)
 * unlikely to happen, but evaluating the actual maximum would require processing, and stopping
 * the whole image until all gradients have been checked */
#define MAX 1445

/* gets the grayscale values, returns a 32 bit value with y and x gradients, each occupying 16 bits*/
void nms_project(stream<uint_32_side_channel> &inStream, stream<uint_8_side_channel> &outStream) {

#pragma HLS INTERFACE axis port=inStream
#pragma HLS INTERFACE axis port=outStream
#pragma HLS INTERFACE ap_ctrl_none port=return

	LineBuffer<WINDOW_DIM,IMG_WIDTH,unsigned int> lineBuff;
	Window<WINDOW_DIM,WINDOW_DIM,unsigned int> window;

	int idxCol=0;
	int pixConvolved =0;

	uint_8_side_channel dataOutSideChannel;
	uint_32_side_channel currPixelSideChannel;

	short g_y1,g_x1,g_y2,g_x2; //partial gradients
	float g, side_1, side_2; //gradients
	float angle;
	unsigned int valin, valin1,valin2;
	unsigned char valout;


// LOOP OPTIMIZED 1 -----------------------------
	int flag = 0;
	int npix = 0;

/*first line*/
	for(int w = 0; w< IMG_WIDTH; w++){

#pragma hls pipeline

		lineBuff.shift_up(w);
		if(flag == 0){
			currPixelSideChannel = inStream.read();
			lineBuff.insert_top((unsigned int) currPixelSideChannel.data, w);
			flag = 1;
		} else {
			lineBuff.insert_top((unsigned int) inStream.read().data, w);
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

#pragma HLS pipeline
			lineBuff.shift_up(w);
			lineBuff.insert_top((unsigned int) inStream.read().data, w);
		}
		for(pix = 0; pix  < IMG_WIDTH; pix++){

#pragma HLS pipeline

/*load window*/
			window.shift_left();
			for(int wrow = 0; wrow < 3; wrow++){
					unsigned int val = (unsigned int)lineBuff.getval(wrow,pixConvolved);
					window.insert(val,wrow,2);
			}

/*find pixel gradient*/
			if (i > 0 && pix >= 2) {
				valin = window.getval(1,1);
				g_y1 = (short) ((valin >> 16) & 0x1FFFF);
				g_x1 = (short) (valin & 0x1FFFF);

/*Other option for gradient would be approximating to the sum of modules*/
				g =hls::sqrt((float)(g_x1*g_x1 + g_y1*g_y1));
/*The following steps can have a huge speed up by approximating the angle values depending
/on the tangent value, with the additional consideration of when gx =0*/
/*find the angle, rotate if negative*/
				angle =hls::atan2((float)g_y1,(float)g_x1) * 180 / PI;

				if (angle < 0) angle+=180;


/*check neighbouring pixels based on angle*/

				if (angle >= 22.5 && angle < 67.5) { //45
					valin1 = window.getval(0,2);
					valin2 = window.getval(2,0);
				} else if (angle >= 67.5 && angle <112.5) { //90
					valin1 = window.getval(0,1);
					valin2 = window.getval(2,1);
				} else if (angle >= 111.5 && angle < 157.5) { //135
					valin1 = window.getval(0,0);
					valin2 = window.getval(2,2);
				} else { //0 || 180
					valin1 = window.getval(1,0);
					valin2 = window.getval(1,2);
				}
/*calculate the neighbouring gradients*/
				g_y1 = (short) ((valin1 >> 16) & 0x1FFFF);
				g_x1 = (short) (valin1 & 0x1FFFF);
				side_1 = hls::sqrt((float)(g_x1*g_x1+ g_y1*g_y1));

				g_y2 = (short) ((valin2 >> 16) & 0x1FFFF);
				g_x2 = (short) (valin2 & 0x1FFFF);
				side_2 = hls::sqrt((float)(g_x2*g_x2 + g_y2*g_y2));
/*if g< side gradients, suppress, else return rescaled gradient*/
				if(g >= side_1 && g>=side_2) {
//					valout =  (unsigned char)(hls::round(g/max*255));
					/*valout is rescaled*/
					valout = (unsigned char)(hls::round(g/MAX*255));
				} else {
					valout = (unsigned char) 0;
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


