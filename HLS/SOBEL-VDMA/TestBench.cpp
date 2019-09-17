#include "split.h"
#include <hls_opencv.h>
void split_ip(AXI_STREAM& input, AXI_STREAM& output, char a);
int main(int argc, char** argv){
	printf("Loading image");
	IplImage* src = cvLoadImage(INPUT_IMAGE);
	IplImage* dst = cvCreateImage(cvGetSize(src), src->depth, src->nChannels);
	printf("Converting to stream");
	AXI_STREAM src_axi, dst_axi;
	IplImage2AXIvideo(src, src_axi);
	printf("Calling function");
	split_ip(src_axi, dst_axi,(char)1);
	printf("Writing image");
	AXIvideo2IplImage(dst_axi, dst);
	cvSaveImage(OUTPUT_IMAGE, dst);
}
