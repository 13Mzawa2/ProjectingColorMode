#pragma once

#include <opencv2\opencv.hpp>
#include <array>

namespace cvutil
{
	static class Colormetric
	{
	public:
		//	pixel-wize operations
		static cv::Vec3d cvtXYZ2CIELAB(cv::Vec3d XYZ, cv::Vec3d XYZ_n);
		static cv::Vec3d cvtCIELAB2XYZ(cv::Vec3d Lab, cv::Vec3d XYZ_n);
		static double deltaE(cv::Vec3d Lab1, cv::Vec3d Lab2);
		static cv::Vec3d cvtXYZ2xyY(cv::Vec3d XYZ);
		static cv::Vec3d cvtxyY2XYZ(cv::Vec3d xyY);
		static cv::Vec3d cvtDisplayBGR2XYZ(cv::Vec3d BGR, cv::Vec3d gamma, cv::Vec3d Lmax, cv::Vec3d offset, cv::Mat cvtMat);
		static cv::Vec3d cvtXYZ2DisplayBGR(cv::Vec3d XYZ, cv::Vec3d gamma, cv::Vec3d Lmax, cv::Vec3d offset, cv::Mat invCvtMat);
		static cv::Vec3d cvtCamBGR2XYZ(cv::Vec3d BGR, cv::Mat cvtMat);
		static std::vector<double> cvtBGR2QuadRGB(cv::Vec3d BGR);
		static cv::Mat cvtMatYBGR2XYZ(std::array<cv::Vec2d, 3> xy);
	};

	//	カメラを簡易二次元色彩計とするためのキャリブレーションエンジン
	class CamColorCalibrator
	{
	private:
		// モデル式パラメータ
		cv::Mat params;

		static cv::Mat cvtMat(const double x[]);
		static cv::Vec3d XYZ_est(const double x[], cv::Vec3d BGR);
		static cv::Vec3d CIELAB_est(const double x[], cv::Vec3d BGR, cv::Vec3d XYZ_n);

		// 最小化するコスト関数
		class CostFunciton : public cv::MinProblemSolver::Function
		{
		public:
			// カメラのBGR値
			std::vector<cv::Vec3d> patchBGRs;
			// 測色値
			std::vector<cv::Vec3d> patchXYZs;
			// 白色点
			cv::Vec3d whitepoint;
			// 目的関数の定義
			double calc(const double* x) const;
			// パラメータベクトルの次元数
			virtual int getDims() const { return 30; }
		};

	public:
		bool calibrated = false;

		cv::Vec3d whiteXYZ;
		
		CamColorCalibrator() = default;
		~CamColorCalibrator() = default;

		// パラメータベクトルの初期化(全て0)
		void paramInit();
		// 最適化実行
		double fit(std::vector<cv::Vec3d> BGRs, std::vector<cv::Vec3d> XYZs);
		// 色差のベクトルを渡す関数
		void deltaEs(std::vector<cv::Vec3d> BGRs, std::vector<cv::Vec3d> XYZs, std::vector<double> &dEs);
		// モデル予測値ベクトル
		void XYZs_est(std::vector<cv::Vec3d> BGRs, std::vector<cv::Vec3d> &XYZs);

		// モデル式
		cv::Mat cvtMat();
		cv::Vec3d XYZ_est(cv::Vec3d BGR);
		cv::Vec3d CIELAB_est(cv::Vec3d BGR);

		// ファイルへ保存・読込
		void read(cv::String path);
		void write(cv::String path);
	};

	//	ディスプレイでXYZを正確に表示するためのキャリブレーションエンジン（ガンマモデル）
	class DisplayColorCalibrator
	{
	private:

	public:
		bool calibrated = false;

		//	parameters
		cv::Vec3d offset;
		std::array<cv::Vec2d, 3> xy_bgr;
		cv::Vec3d Lmax;
		cv::Vec3d gamma;

		//	最適化実行
		double fit(std::vector<double> BGR_vals, std::array<std::vector<cv::Vec3d>, 3> xyLs);

		//	モデル式
		cv::Mat cvtMat() { return Colormetric::cvtMatYBGR2XYZ(xy_bgr); };
		cv::Vec3d XYZ_est(cv::Vec3d BGR) { return Colormetric::cvtDisplayBGR2XYZ(BGR, gamma, Lmax, offset, cvtMat()); };

		//	モデル予測値ベクトル
		void XYZs_est(std::vector<cv::Vec3d> BGRs, std::vector<cv::Vec3d> &XYZs);
	};
}