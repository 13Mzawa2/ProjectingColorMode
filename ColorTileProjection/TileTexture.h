#pragma once
#include <opencv2\opencv.hpp>
#include <array>
#include "Colormetric.h"

class TileTexture
{
public:
	TileTexture();
	~TileTexture();

	//	color calibration data
	//	(b1(x,y,L),b2(x,y,L),...),(g1(x,y,L),...),(r1(x,y,L),...)
	std::array<std::vector<cv::Vec3d>, 3> bgr_xyL;
	cvutil::DisplayColorCalibrator calib;
};
