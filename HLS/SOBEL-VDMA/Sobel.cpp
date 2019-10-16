#include "split.h"
#define IMG_HEIGHT 720
#define IMG_WIDTH 1280
#define PI 3,1452
#define MAXABS 2040 //rescaling value in order to fit gradient in 8 bit
#define MAX 1445
#include <ap_axi_sdata.h>
typedef hls::Mat<720,1280, HLS_32S> RGB_IMAGE32;
typedef hls::Scalar<1, int> OUT_PIX;


/*
 * Gray function
 * Converts a RGB image to Gray
 */

coeff_type const1 = 0.114;
coeff_type const2 = 0.587;
coeff_type const3 = 0.2989;
short add(short a, short b);


/*Gaussian blur 3x3, with grayscale included*/
void blur(RGB_IMAGE &img_in, RGB_IMAGE &image_out) {
		RGB_PIX pin;
		RGB_PIX pout;
		hls::LineBuffer<3,1280,unsigned char> lineBuff;
		hls::Window<3,3,unsigned char> window;
		int idxCol=0;
		int idxRow=0;
		unsigned char valout;
		int pixConvolved =0;

		L_row:	for (int row  =0; row<IMG_HEIGHT; row++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=720
			L_col: for(int col = 0; col < IMG_WIDTH; col++) {
//#pragma HLS unroll factor = 10 // BEFORE THIS 2808735
#pragma HLS LOOP_TRIPCOUNT min=1 max=1280
#pragma HLS pipeline II=1
#pragma HLS loop_flatten
				img_in >> pin;
				lineBuff.shift_up(col);
				char gray1 = const1 * pin.val[0];
				char gray2 = const2 * pin.val[1];
				char grayPart = gray1 + gray2;
				char gray3 = const3 * pin.val[2];
				char grayPart2 = grayPart + gray3;
				lineBuff.insert_top(grayPart2,col);
			}

			L_coll: for(int col =0; col < IMG_WIDTH; col++){
#pragma HLS loop_flatten
#pragma HLS pipeline II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=1280
				if (row >1){
					window.shift_left();
					for(int wrow = 0; wrow < 3; wrow++){
#pragma HLS loop_flatten
#pragma HLS pipeline II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=3
							short val = (short)lineBuff.getval(wrow,pixConvolved);
							window.insert(val,wrow,2);
					}
					unsigned int g = 0;
					unsigned char u0,u1,u2,u3,u4,u5,u6,u7,u8;
					unsigned char sum1,sum2,sum3,sum4;
					valout = 0;

					if ( col >= 2) {
						u0 = window.getval(0,0)/16;
						u1 = window.getval(0,1)/8;
						u2 = window.getval(0,2)/16;
						u3 = window.getval(1,0)/8;
						u4 = window.getval(1,1)/4;
						u5 = window.getval(1,2)/8;
						u6 = window.getval(2,0)/16;
						u7 = window.getval(2,1)/8;
						u8 = window.getval(2,2)/16;
						sum1 = u0 + u8;
						sum2 = u1 + u7;
						sum3 = u2 + u6;
						sum4 = u3 + u5;
						valout = sum1 +sum2 + sum3 + sum4 + u4;
						pixConvolved++;
					}
					if(pixConvolved == 0 || pixConvolved == IMG_WIDTH -2){
						pixConvolved = 0;
					}
				} else {
					valout =0;
				}
				pout.val[0] =  valout;
				pout.val[1] =  valout;
				pout.val[2] =  valout;
				image_out << pout;
		}
	}
}

void sobel(RGB_IMAGE &img_in, RGB_IMAGE32 &image_out) {
#pragma HLS inline
		RGB_PIX pin;
		OUT_PIX pout;
		hls::LineBuffer<3,1280,unsigned char> lineBuff;
		hls::Window<3,3,short> window;
		int idxCol=0;
		int idxRow=0;
		int valout;
		int pixConvolved =0;

		L_row:	for (int row  =0; row<IMG_HEIGHT; row++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=720
			L_col: for(int col = 0; col < IMG_WIDTH; col++) {
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=1 max=1280
#pragma HLS loop_flatten
				img_in >> pin;
				lineBuff.shift_up(col);
				lineBuff.insert_top(pin.val[0],col);
			}

			L_coll: for(int col =0; col < IMG_WIDTH; col++){
#pragma HLS loop_flatten
#pragma HLS pipeline II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=1280
				if (row >1){
					window.shift_left();
					for(int wrow = 0; wrow < 3; wrow++){
#pragma HLS loop_flatten
#pragma HLS pipeline II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=3
							short val = (short)lineBuff.getval(wrow,pixConvolved);
							window.insert(val,wrow,2);
					}
					unsigned int g = 0;
					unsigned short ux,uy;
					valout = 0;

					if ( col >= 2) {
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


						valout =(int)( (g << 16) |  ux);
						pixConvolved++;
					}
					if(pixConvolved == 0 || pixConvolved == IMG_WIDTH -2){
						pixConvolved = 0;
					}
				} else {
					valout =0;
				}
				pout.val[0] = (int) valout;
				image_out << pout;
		}
	}
}
short add(short a, short b){
#pragma HLS inline
	return a+b;
}
/*non maximum suppression function. The approximations used are: calculation of the module as sum of absolute value of components
*and definition of gradient direction depending on tangent range*/

void nms(RGB_IMAGE32 &img_in, RGB_IMAGE &image_out) {
#pragma HLS inline
	OUT_PIX pin;
	RGB_PIX pout;

	hls::LineBuffer<3,IMG_WIDTH, unsigned int> lineBuff;
	hls::Window<3,3,unsigned int> window;
	int idxCol=0;
	int idxRow=0;
	int pixConvolved =0;


	unsigned int g_y1,g_x1,g_y2,g_x2; //partial gradients
	float g, side_1, side_2; //gradients
	float angle;
	float tan;
	 int valin, valin1,valin2;
	unsigned char valout;


	for (int row  =0; row<IMG_HEIGHT; row++) {
#pragma HLS loop_flatten
#pragma HLS LOOP_TRIPCOUNT min=1 max=720
		L_col: for(int col = 0; col < IMG_WIDTH; col++) {
#pragma HLS loop_flatten
#pragma HLS pipeline II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=1280
			img_in >> pin;
			lineBuff.shift_up(col);
			lineBuff.insert_top((unsigned int) pin.val[0],col);
		}

		L_coll: for(int col =0; col < IMG_WIDTH; col++){
			if (row >=1){
				window.shift_left();
#pragma HLS pipeline
#pragma HLS loop_flatten
#pragma HLS LOOP_TRIPCOUNT min=1 max=1280
				for(int wrow = 0; wrow < 3; wrow++){
#pragma HLS loop_flatten
#pragma HLS pipeline II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=3
						int val = (unsigned int)lineBuff.getval(wrow,pixConvolved);
						window.insert(val,wrow,2);
				}

				valout = 0;
				if ( col >= 2) {
					valin = window.getval(1,1);
					g_y1 = (unsigned int) ((valin >> 16) & 0x1FFFF);
					g_x1 = (unsigned int) (valin & 0x1FFFF);

					g = hls::abs((float)g_x1) + hls::abs((float)g_y1);
	/*find the tangent, in the case of tan=inf, set to "high" value*/
					if (g_x1 == 0) {
						tan = 90;
					} else {
						tan = g_y1/g_x1;
					}
				


	/*check neighbouring pixels based on angle*/
					if (tan == 90) {
						valin1 = window.getval(0,1);
						valin2 = window.getval(2,1);
					} else if ((tan >= 0 && tan <= 0,4142) || (tan >= -0,4142 && tan <= 0) ) {
						valin1 = window.getval(1,0);
						valin2 = window.getval(1,2);
					} else if ((tan >= 0,4142 && tan <= 2,4142) ) {
						valin1 = window.getval(0,2);
						valin2 = window.getval(2,0);
					} else if ((tan >= -2,4142 && tan <= -0,4142)) {
						valin1 = window.getval(0,0);
						valin2 = window.getval(2,2);
					} else {
						valin1 = window.getval(0,1);
						valin2 = window.getval(2,1);
					}
	/*calculate the neighbouring gradients*/
					g_y1 = (unsigned int) ((valin1 >> 16) & 0x1FFFF);
					g_x1 = (unsigned int) (valin1 & 0x1FFFF);
					//side_1 = hls::sqrt((float)(g_x1*g_x1+ g_y1*g_y1));
					side_1 = hls::abs((float)g_x1) + hls::abs((float)g_y1);
					g_y2 = (unsigned int) ((valin2 >> 16) & 0x1FFFF);
					g_x2 = (unsigned int) (valin2 & 0x1FFFF);
					//side_2 = hls::sqrt((float)(g_x2*g_x2 + g_y2*g_y2));
					side_2 = hls::abs((float)g_x2) + hls::abs((float)g_y2);
	/*if g< side gradients, suppress, else return rescaled gradient*/
					if(g >= side_1 && g>=side_2) {
	
	/*valout is rescaled*/
						double d = (hls::round((g/MAXABS)*255));
						valout = (unsigned char) d;
					} else {
						valout = (unsigned char) 0;
					}

					pixConvolved++;
				}
				if(pixConvolved == 0 || pixConvolved == IMG_WIDTH -2){
					pixConvolved = 0;
				}
			} else {
				valout =0;
			}

			pout.val[0] = valout;
			pout.val[1] = valout;
			pout.val[2] = valout;

			image_out << pout;
		}
	}
}

void split_ip(AXI_STREAM& in_data, AXI_STREAM& out_data, char direction) {

#pragma HLS INTERFACE axis register port=in_data
#pragma HLS INTERFACE axis register port=out_data
#pragma HLS inline
#pragma HLS dataflow
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE s_axilite port=direction


    RGB_IMAGE img_0(720, 1280);
    RGB_IMAGE img_1(720, 1280);
    RGB_IMAGE32 img_2(720, 1280);
    RGB_IMAGE img_3(720,1280);
	// Convert AXI4 Stream data to hls::mat format
	hls::AXIvideo2Mat(in_data, img_0);

	//call the split function
	//RGB2Gray(img_0, img_1);
	blur(img_0, img_1),
	sobel(img_1, img_2);
	nms(img_2,img_3);
//	onlySobel(img_1,img_3);
	//Convert the mat to Axi video stream
	hls::Mat2AXIvideo(img_3, out_data);
}
