/*
 * RobotinoVision.cpp
 *
 *  Created on: 15/07/2014
 *      Author: adrianohrl@unifei.edu.br
 */

#include "RobotinoVision.h"

RobotinoVision::RobotinoVision()
	: it_(nh_),
	  threshVal_(250),
	  dilationSize_(2),
	  redThreshMinVal1_(0),
	  redThreshMaxVal1_(10),
	  redThreshMinVal2_(228),
	  redThreshMaxVal2_(255),
	  yellowThreshMinVal_(15),
	  yellowThreshMaxVal_(55),
	  greenThreshMinVal_(58),
	  greenThreshMaxVal_(96)
{
	image_sub_ = it_.subscribe("image_raw", 1, &RobotinoVision::imageCallback, this);
	save_srv_ = nh_.advertiseService("save_image", &RobotinoVision::saveImage, this);
	imgRGB_ = Mat(320, 240, CV_8UC3, Scalar::all(0));
	//namedWindow(OPENCV_WINDOW);

	threshV_ = 70;
	erosionSize_ = 2;
	threshS_ = 40;
	closeSize1_ = 6;
	openSize1_ = 9;
}

RobotinoVision::~RobotinoVision()
{
	image_sub_.shutdown();
	save_srv_.shutdown();
	//destroyWindow(OPENCV_WINDOW);
}

bool RobotinoVision::spin()
{
	ros::Rate loop_rate(30);
	while(nh_.ok())
	{
		ros::spinOnce();
		loop_rate.sleep();
		//char* imageName = "../../samples/pucks.jpg";
		//imgRGB_ = readImage(imageName);
	}
	return true;
}

cv::Mat RobotinoVision::readImage(char* imageName)
{
	Mat image;
	image = imread(imageName, 1);
	
	if (!image.data)
	{
		ROS_ERROR("No image data!!!");
		return Mat(320, 240, CV_8UC3, Scalar::all(0));
	}
	
	return image;
}

bool RobotinoVision::saveImage(robotino_vision::SaveImage::Request &req, robotino_vision::SaveImage::Response &res)
{
	imwrite(req.image_name.c_str(), imgRGB_);
	return true;
}

void RobotinoVision::imageCallback(const sensor_msgs::ImageConstPtr& msg)
{
	cv_bridge::CvImagePtr cv_ptr;
	try
	{
		cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::RGB8);
	}
	catch (cv_bridge::Exception& e)
	{
		ROS_ERROR("cv_bridge exception: %s", e.what());
		return;
	}
	imgRGB_ = cv_ptr->image;
	unsigned char initialRangeValue = 90, rangeWidth = 20;
	processImagePucks(initialRangeValue, rangeWidth);
}

void RobotinoVision::processImagePucks(unsigned char initialRangeValue, unsigned char rangeWidth)
{
	// para visualização da imagem no visor em BGR	
	Mat imgBGR;
	cvtColor(imgRGB_, imgBGR, CV_RGB2BGR);
	imshow("BRG Model", imgBGR);

	// convertendo de RGB para HSV
	Mat imgHSV;
	cvtColor(imgRGB_, imgHSV, CV_RGB2HSV);
	//imshow("HSV Model", imgHSV);

	// separando a HSV 
	Mat splittedImgHSV[3];
	split(imgHSV, splittedImgHSV);
	//Mat imgH = splittedImgHSV[0];
	//imshow("Hue", imgH);
	Mat imgS = splittedImgHSV[1];
	//imshow("Saturation", imgS);
	Mat imgV = splittedImgHSV[2];
	//imshow("Value", imgV);

	// fazendo threshold da imagem V
	Mat threshImgV;
	int maxVal = 255;
	threshold(imgV, threshImgV, threshV_, maxVal, THRESH_BINARY);
	//namedWindow("Value Window", 1);
	//createTrackbar("Value threshold value:", "Value Window", &threshV_, 255);
	//imshow("Value Window", threshImgV);

	// fazendo erosão na imagem acima
	Mat element1 = getStructuringElement(MORPH_RECT, Size(2 * erosionSize_ + 1, 2 * erosionSize_ + 1), Point(erosionSize_, erosionSize_));
	Mat blackMask;
	erode(threshImgV, blackMask, element1);
	//namedWindow("Eroded Value", 1);
	//createTrackbar("Erosion Size", "Eroded Value", &erosionSize_, 10);
  	//imshow("Eroded Value", blackMask);
	
	// convertendo de RGB para HSV
	Mat imgHSL;
	cvtColor(imgRGB_, imgHSL, CV_RGB2HLS);
	//imshow("HSL Model", imgHSL);

	// fazendo threshold da imagem S
	Mat threshImgS;
	maxVal = 255;
	threshold(imgS, threshImgS, threshS_, maxVal, THRESH_BINARY);
	//namedWindow("Saturation Window", 1);
	//createTrackbar("Value threshold value:", "Saturation Window", &threshS_, 255);
	//imshow("Saturation Window", threshImgS); 

	// fechando buracos
	Mat element2 = getStructuringElement(MORPH_CROSS, Size(2 * closeSize1_ + 1, 2 * closeSize1_ + 1), Point(closeSize1_, closeSize1_));
	Mat imgClosedS;
	morphologyEx(threshImgS, imgClosedS, 3, element2);
	//namedWindow("Closed Image", 1);
	//createTrackbar("Close Size", "Closed Image", &closeSize1_, 10);
  	//imshow("Closed Image", imgClosedS);

	// filtro de partícula pequenas
	Mat element3 = getStructuringElement(MORPH_CROSS, Size(2 * openSize1_ + 1, 2 * openSize1_ + 1), Point(openSize1_, openSize1_));
	Mat puckMask;
	morphologyEx(threshImgS, puckMask, 2, element3);
	//namedWindow("Open Image", 1);
	//createTrackbar("Open Size", "Open Image", &openSize1_, 10);
  	//imshow("Open Image", puckMask);

	// separando a HSL 
	Mat splittedImgHSL[3];
	split(imgHSL, splittedImgHSL);
	Mat imgH = splittedImgHSL[0];
	imshow("Hue", imgH);
	//Mat imgS = splittedImgHSL[1];
	//imshow("Saturation", imgS);
	//Mat imgL = splittedImgHSL[2];
	//imshow("Lightness", imgL);

	// 
	Mat imgH_new1 = imgH - initialRangeValue;
	imshow("New Hue 1", imgH_new1);

	Mat imgH_new2 = imgH + (255 - initialRangeValue); 
	//imshow("New Hue 2", imgH_new2);
	Mat imgH_new3;
	threshold(imgH_new2, imgH_new3, 254, 255, THRESH_BINARY);
	imgH_new3 = 255 - imgH_new3; 
	imshow("aoskdoak", imgH_new3);
	bitwise_and(imgH_new2, imgH_new3, imgH_new2);
	imshow("New Hue 2", imgH_new2);
	
	Mat imgH_new4 = imgH_new1 + imgH_new2;
	imshow("New Hue 4", imgH_new4);

	waitKey(80);
}

void RobotinoVision::processImageLampPost()
{
	Mat imgBGR;
	cvtColor(imgRGB_, imgBGR, CV_RGB2BGR);
	imshow("BRG Model", imgBGR);
	Mat imgHSL;
	cvtColor(imgRGB_, imgHSL, CV_RGB2HLS);
	imshow("HSL Model", imgHSL);
	Mat splittedImgHSL[3];
	split(imgHSL, splittedImgHSL);
	Mat imgH = splittedImgHSL[0];
	imshow("Hue", imgH);
	Mat imgS = splittedImgHSL[1];
	imshow("Saturation", imgS);
	Mat imgL = splittedImgHSL[2];
	imshow("Lightness", imgL);
	
	Mat threshImgL;
	namedWindow("Lightness Window", 1);
	createTrackbar("Lightness threshold value:", "Lightness Window", &threshVal_, 255);
	int maxVal = 255;
	threshold(imgL, threshImgL, threshVal_, maxVal, THRESH_BINARY);
//	std::vector<std::vector<Point>> contours;
//	std::vector<Vec4i> hierarchy;
//	findContours(threshImgL, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
//	Mat imgBorders = Mat::zeros(threshImgL.size(), CV_8U);
//	for (int i = 0; i < contours.size(); i++)
//		drawContours(imgBorders, contours, i, Scalar(255), 2, 8, hierarchy, 0, Point());
//	imshow("Contours", imgBorders);
	//imshow("Lightness Window", threshImgL);
	
	/*const int maskSize = 7;
	int mask[maskSize][maskSize] = {{0, 0, 0, 0, 0, 0, 0},
					{0, 0, 0, 0, 0, 0, 0},
					{0, 0, 0, 0, 0, 0, 0}, 
					{1, 1, 1, 0, 1, 1, 1},
					{0, 0, 0, 0, 0, 0, 0},
					{0, 0, 0, 0, 0, 0, 0}, 
					{0, 0, 0, 0, 0, 0, 0}};  
	Mat kernel = Mat(maskSize, maskSize, CV_8U, (void*) mask, maskSize);*/
	namedWindow("Dilated Lightness", 1);
	createTrackbar("Dilation Size", "Dilated Lightness", &dilationSize_, 10);
	Mat element = getStructuringElement(MORPH_RECT, Size(2 * dilationSize_ + 1, 2 * dilationSize_ + 1), Point(dilationSize_, dilationSize_)); 
	/*std::sstream == "";
	for (t_size i = 0; i < element.rows; i++)
	{
		for (t_size j = 0; j < element.columns; j++)
			s << 
		s << std::endl;
	}	*/

	Mat dilatedImgL;
	dilate(threshImgL, dilatedImgL, element);
	imshow("Dilated Lightness", dilatedImgL);
	
	Mat mask = dilatedImgL - threshImgL;
	imshow("Mask", mask);
	mask /= 255;
/*
	Mat imgH1, imgH2; 
	imgH.copyTo(imgH1);//, CV_8U);
	Scalar s1 = Scalar(25);
	imgH1 += s1;
	//std::cout << imgH1;
	imgH.copyTo(imgH2);//, CV_8U;
	Scalar s2 = Scalar(230);
	imgH2 -= s2;
	Mat newImgH = imgH1 + imgH2;*/
	vector<Mat> masks;
	masks.push_back(mask);
	masks.push_back(mask);
	masks.push_back(mask);
	Mat imgMask;
	merge(masks, imgMask);
	Mat maskedImgBGR;
	maskedImgBGR = imgBGR.mul(imgMask);
	Mat finalImgH = imgH.mul(mask); 
	imshow("Final Filter", maskedImgBGR);

	Mat imgRed, imgRed1, imgRed2, imgRed3, imgRed4;
	//int redThreshMinVal1 = 0;
	//int redThreshMaxVal1 = 10;
	namedWindow("Red Range", 1);
	createTrackbar("Range1MinVal", "Red Range", &redThreshMinVal1_, 255);
	createTrackbar("Range1MaxVal", "Red Range", &redThreshMaxVal1_, 255);
	int redMaxVal = 1;
	threshold(finalImgH, imgRed1, redThreshMinVal1_, redMaxVal, THRESH_BINARY);
	threshold(finalImgH, imgRed2, redThreshMaxVal1_, redMaxVal, THRESH_BINARY_INV);
	imgRed3 = imgRed1.mul(imgRed2); 
	//int redThreshMinVal2 = 228;
	//int redThreshMaxVal2 = 255;
	createTrackbar("Range2MinVal", "Red Range", &redThreshMinVal2_, 255);
	createTrackbar("Range2MaxVal", "Red Range", &redThreshMaxVal2_, 255);
	threshold(finalImgH, imgRed1, redThreshMinVal2_, redMaxVal, THRESH_BINARY);
	threshold(finalImgH, imgRed2, redThreshMaxVal2_, redMaxVal, THRESH_BINARY_INV);
	imgRed4 = imgRed1.mul(imgRed2);
	imgRed = (imgRed3 + imgRed4) * 255;
	imshow("Red Range", imgRed);

	Mat imgYellow, imgYellow1, imgYellow2;
	//int yellowThreshMinVal = 15;
	//int yellowThreshMaxVal = 55;
	namedWindow("Yellow Range", 1);
	createTrackbar("RangeMinVal", "Yellow Range", &yellowThreshMinVal_, 255);
	createTrackbar("RangeMaxVal", "Yellow Range", &yellowThreshMaxVal_, 255);
	int yellowMaxVal = 1;
	threshold(finalImgH, imgYellow1, yellowThreshMinVal_, yellowMaxVal, THRESH_BINARY);
	threshold(finalImgH, imgYellow2, yellowThreshMaxVal_, yellowMaxVal, THRESH_BINARY_INV);
	imgYellow = imgYellow1.mul(imgYellow2) * 255;
	imshow("Yellow Range", imgYellow);
	
	Mat imgGreen, imgGreen1, imgGreen2;
	//int greenThreshMinVal = 58;
	//int greenThreshMaxVal = 96;
	namedWindow("Green Range", 1);
	createTrackbar("RangeMinVal", "Green Range", &greenThreshMinVal_, 255);
	createTrackbar("RangeMaxVal", "Green Range", &greenThreshMaxVal_, 255);
	int greenMaxVal = 1;
	threshold(finalImgH, imgGreen1, greenThreshMinVal_, greenMaxVal, THRESH_BINARY);
	threshold(finalImgH, imgGreen2, greenThreshMaxVal_, greenMaxVal, THRESH_BINARY_INV);
	imgGreen = imgGreen1.mul(imgGreen2) * 255;
	imshow("Green Range", imgGreen);
	
	waitKey(80);
}

