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

	//	�J�������ȈՓ񎟌��F�ʌv�Ƃ��邽�߂̃L�����u���[�V�����G���W��
	class CamColorCalibrator
	{
	private:
		// ���f�����p�����[�^
		cv::Mat params;

		static cv::Mat cvtMat(const double x[]);
		static cv::Vec3d XYZ_est(const double x[], cv::Vec3d BGR);
		static cv::Vec3d CIELAB_est(const double x[], cv::Vec3d BGR, cv::Vec3d XYZ_n);

		// �ŏ�������R�X�g�֐�
		class CostFunciton : public cv::MinProblemSolver::Function
		{
		public:
			// �J������BGR�l
			std::vector<cv::Vec3d> patchBGRs;
			// ���F�l
			std::vector<cv::Vec3d> patchXYZs;
			// ���F�_
			cv::Vec3d whitepoint;
			// �ړI�֐��̒�`
			double calc(const double* x) const;
			// �p�����[�^�x�N�g���̎�����
			virtual int getDims() const { return 30; }
		};

	public:
		bool calibrated = false;

		cv::Vec3d whiteXYZ;
		
		CamColorCalibrator() = default;
		~CamColorCalibrator() = default;

		// �p�����[�^�x�N�g���̏�����(�S��0)
		void paramInit();
		// �œK�����s
		double fit(std::vector<cv::Vec3d> BGRs, std::vector<cv::Vec3d> XYZs);
		// �F���̃x�N�g����n���֐�
		void deltaEs(std::vector<cv::Vec3d> BGRs, std::vector<cv::Vec3d> XYZs, std::vector<double> &dEs);
		// ���f���\���l�x�N�g��
		void XYZs_est(std::vector<cv::Vec3d> BGRs, std::vector<cv::Vec3d> &XYZs);

		// ���f����
		cv::Mat cvtMat();
		cv::Vec3d XYZ_est(cv::Vec3d BGR);
		cv::Vec3d CIELAB_est(cv::Vec3d BGR);

		// �t�@�C���֕ۑ��E�Ǎ�
		void read(cv::String path);
		void write(cv::String path);
	};

	//	�f�B�X�v���C��XYZ�𐳊m�ɕ\�����邽�߂̃L�����u���[�V�����G���W���i�K���}���f���j
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

		//	�œK�����s
		double fit(std::vector<double> BGR_vals, std::array<std::vector<cv::Vec3d>, 3> xyLs);

		//	���f����
		cv::Mat cvtMat() { return Colormetric::cvtMatYBGR2XYZ(xy_bgr); };
		cv::Vec3d XYZ_est(cv::Vec3d BGR) { return Colormetric::cvtDisplayBGR2XYZ(BGR, gamma, Lmax, offset, cvtMat()); };

		//	���f���\���l�x�N�g��
		void XYZs_est(std::vector<cv::Vec3d> BGRs, std::vector<cv::Vec3d> &XYZs);
	};
}