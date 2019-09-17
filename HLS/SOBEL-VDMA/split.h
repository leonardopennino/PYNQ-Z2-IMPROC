#ifndef _EXAMPLE_SPLIT_H_
#define _EXAMPLE_SPLIT_H_

#include "hls_video.h"

typedef hls::stream< ap_axiu<24,1,1,1> > AXI_STREAM;
typedef hls::Mat<720,1280, HLS_8UC3> RGB_IMAGE;
typedef hls::Scalar<3, unsigned char> RGB_PIX;
short add(short a, short b);
typedef ap_fixed<10,2, AP_RND, AP_SAT> coeff_type;

#define INPUT_IMAGE		"C:\\Users\\leona\\stream2016\\canny.jpg"
#define OUTPUT_IMAGE 	"C:\\Users\\leona\\stream2016\\test_output_1080p.jpg"

void split_ip(AXI_STREAM& input, AXI_STREAM& output, int a);
void onlySobel(RGB_IMAGE &img_in,RGB_IMAGE &image_out);

#endif
