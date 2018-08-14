#pragma once
#include <Siv3DAddon\OpenCV.hpp>
#include <Siv3D.hpp>

class TextureNoise
{
public:

	TextureNoise()
	{
	}

	~TextureNoise()
	{
	}

	static Image generate(Size sz)
	{
		cv::Mat noise(sz.y, sz.x, CV_8UC1);
		cv::RNG rng(cvGetTickCount());
		rng.fill(noise, cv::RNG::NORMAL, 128, 10);
		cv::cvtColor(noise, noise, cv::COLOR_GRAY2BGR);
		//cv::GaussianBlur(noise, noise, cv::Size(3, 3), 0.5);
		//	opencv -> siv3d
		Image img = OpenCV::ToImage(cv::Mat_<cv::Vec3b>(noise));
		return img;
	}
};

