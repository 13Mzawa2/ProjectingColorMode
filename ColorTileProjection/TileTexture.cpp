#include "TileTexture.h"



TileTexture::TileTexture()
{
}


TileTexture::~TileTexture()
{
}

cv::Mat TileTexture::makeTileTexture(cv::Mat colors, cv::Size tex_sz, cv::Size Gaussian_sz, double Gaussian_sd, bool invert)
{
	//	colors内はxyL -> キャリブレーションを基にBGRに変換
	cv::Mat colors_BGR(colors.size(), CV_8UC3);
	auto invMat = calib.cvtMat().inv();
	colors_BGR.forEach<cv::Vec3b>([&](cv::Vec3b &c, const int *pos)->void {
		auto XYZ = cvutil::Colormetric::cvtxyY2XYZ(colors.at<cv::Vec3d>(pos[0],pos[1]));
		auto BGR = cvutil::Colormetric::cvtXYZ2DisplayBGR(
			XYZ, calib.gamma, calib.Lmax, calib.offset, invMat);
		c = cv::Vec3b(BGR);
	});
	//	colorsに互い違いに白色を埋めていく
	cv::Mat teximg(colors.rows, colors.cols * 2, CV_8UC3, cv::Scalar::all(255));
	colors_BGR.forEach<cv::Vec3b>([&](cv::Vec3b &c, const int *pos)->void {
		int shift = (invert) ? 1 : 0;
		if (pos[0] % 2 == 0) {
			teximg.at<cv::Vec3b>(pos[0], pos[1] * 2 + (1-invert)) = c;
		}
		else {
			teximg.at<cv::Vec3b>(pos[0], pos[1] * 2 + invert) = c;
		}
	});
	cv::copyMakeBorder(teximg, teximg, 1, 1, 1, 1, cv::BORDER_CONSTANT, cv::Scalar::all(255));
	//	テクスチャを指定サイズに拡大
	cv::Mat temp;
	cv::resize(teximg, temp, tex_sz, 0, 0, cv::INTER_NEAREST);
	//	テクスチャを指定サイズでぼかし
	if (Gaussian_sd > 0.0) {
		cv::GaussianBlur(temp, temp, Gaussian_sz, Gaussian_sd);
	}
	return temp;
}

cv::Mat TileTexture::transformedTexture(cv::Mat src, cv::Size proSize, std::vector<cv::Point> corners)
{
	std::vector<cv::Point> corners_org = {
		cv::Point(0,0),
		cv::Point(src.cols, 0),
		cv::Point(src.cols, src.rows),
		cv::Point(0, src.rows)
	};
	auto homography = cv::findHomography(corners_org, corners);
	cv::Mat dst;
	cv::warpPerspective(src, dst, homography, proSize, 1, cv::BORDER_CONSTANT, cv::Scalar::all(255));

	return dst;
}
