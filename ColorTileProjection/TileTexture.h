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
	std::array<std::vector<cv::Vec3d>, 3> bgr_xyLs;
	std::vector<double> BGR_vals;
	cvutil::DisplayColorCalibrator calib;

	cv::Mat makeTileTexture(cv::Mat colors, cv::Size tex_sz, cv::Size Gaussian_sz = cv::Size(), double Gaussian_sd = 0, bool invert = false);
	cv::Mat transformedTexture(cv::Mat src, cv::Size proSize, std::vector<cv::Point> corners);
};

