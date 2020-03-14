# PYNQ-Z2-IMPROC
IMAGE PROCESSING ON XILINX PYNQ Z2 (CANNY, SOBEL)

## Introduction:
We are Giacomo Zamprogno, Leonardo Penninno and Vincenzo De Marco, a team of student from Politecnico of Turin that took part on a computer vision project during our course in Computer Architecture.
The aim of the project was implementing the Canny algorithm for image processing on the PYNQ-Z2 board.  The development tools used were Vivado HLSfor the ip modules creation and Vivado Design for the overlay design.

## The algorithm:
The  canny  algorithm  is  an  image  processing  algorithm  for  edge  detection  inimages, and its description can be split into five phases:
* Grayscale conversion:  the image is converted into the grayscale counter-part, since there is no need for the color channels information;
* Gaussian blurring:  an initial gaussian filter is convolved with the image,in order to reduce noise for the future gradient detection;
* Sobel  gradient  extraction:   the  image  is  convolved  with  x  and  y  Sobelfilters,  in  order  to  compute,  for  each  pixel,  angle  and  magnitude  of  thegradient vector;
* Non maximum suppression: the gradient image is further filtered to ”thin”the lines, suppressing each gradient which is not a local maximum amongthe neighbours in its direction;
* Hysteresis: the final step of the algorithm is to set two thresholds, low andhigh,  suppress any pixel below the ”low” threshold,  and keep any pixelabove  the  ”high”  threshold.   Pixel  in  the  between  of  the  thresholds  arekept only if neighbouring ”high” pixels.

Due to the different kind of resources available in the dma and vdma imple-mentation, not all function have been implemented for the project, specifically,the vdma implementation does not use the hysteresis, and the dma implementation uses built-in grayscale and blur.

## Sobel: 
There are two kernels for Sobel extraction: [1,0,-1; 2,0,-2; 1,0,-1] and [1,2,1; 0,0,0; -1,-2,-1]

Respectively for Gx and Gy components of the gradient.  The image is filtered on a 3x3 window, and due to limited size of memory, a 3-lines linebuffer is used.The linebuffer technique is also used in most of the other IPs, due to the same reasons. For each pixel, the two components of the gradient are calculated at the same time, and then forwarded in the stream to the non maximum suppression phase.
A  burst  tecnique  allows  to  improve  the  performances,  since  each  line  is  first loaded in the linebuffer and then the window shifts on the whole line, following pipelining instructions.  The two gradient components are stored and sent on 32 bits (16 for each) using bit shifting and bitwise logic, to facilitate the flow into the stream.

## Non Maximum Suppression: 

Non maximum suppression is implemented in a similar way to Sobel, using the linebuffer and window techniques.  The function receives 32 bits as input (largest possible data width), 16 bits for each gradient component, which are later converted in their ”short” counterpart.  From the components, the magnitude and the  gradient  are  calculated.   Different  techniques  can  be  used,  depending  on performance and precision.  The dma version uses a square root magnitude calculation, and arctan angle calculation.  In order to increase performances, the vdma version uses approximations:  sum of absolute values for the magnitude and tangent values for the angle, since only the direction of the neighbouring pixel are of interest.  Dma version could probably be converted as well,  since the  gained  precision  is  not  fully  exploited;  but  since  the  bottleneck  on  ps-pl communication is orders of magnitude slower, improving performance on that side would not be meaningful. The actual function compares each gradient with the gradient of its neighbouring pixels on the direction of the gradient, and suppress it in the case it is nota local maximum.

## Hysteresis

Hysteresis consists of an additional suppression of possible edge values when they are below an user defined threshold, while keeping such pixels in the case they are above an ”high” threshold, considering them sure edge members.  Gradients in between are kept only in the case they are neighbours with a ”high” pixel.  In the case of the algorithm, the function was implemented in the dma version, but itw as not particularly meaninguful for the scope of the project.  Their usefulness depends in fact on the usage of the canny algorithm, since it can reduce noise from not lowly defined edges.

## Greyscale, blur

Grayscale and blur were not the main focus of the project, but they were indeed implemented in the vdma version, since the usage of built-in function was not feasible when the flow of data was coming directly from the hdmi port.

## Results

The reference point for the results was on the software implementation of canny: 0.23  seconds  for  the  whole  process,  0.077  for  the  single  canny  function.   The hardware  accelerated  function  indeed  presents  an  improvement  in  the  single function, taking 0.014 seconds, which is 0.15 as a whole.  The improvement is present,  but not significant,  so the attempt to use vdmi was justified.  Vdma timing calculation are less precise, since there is a timing controller in the overla ythat links the speed and delay to the hdmi refresh rate; but the fact that the overlay manages to keep up with its refresh rate suggests a huge improvement over the low fps results of both the software and hardware dma solutions.  The vdma  version  indeed  presents  a  visual  bug  (four  unexpected  lines  of  pixels) when working at normal refresh rate. This can be solved changing the refresh rate to 50hz.
