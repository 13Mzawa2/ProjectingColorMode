#include "Colormetric.h"


cv::Vec3d cvutil::Colormetric::cvtXYZ2CIELAB(cv::Vec3d XYZ, cv::Vec3d XYZ_n)
{
	//	Lab色空間の1/3乗式
	auto func = [](double x) {
		return (x > 0.008856) ?
			pow(x, 0.33333)
			: 7.787037*x + 0.137931;
	};
	return cv::Vec3d(
		116 * func(XYZ[1] / XYZ_n[1]) - 16,
		500 * (func(XYZ[0] / XYZ_n[0]) - func(XYZ[1] / XYZ_n[1])),
		200 * (func(XYZ[1] / XYZ_n[1]) - func(XYZ[2] / XYZ_n[2]))
	);
}

cv::Vec3d cvutil::Colormetric::cvtCIELAB2XYZ(cv::Vec3d Lab, cv::Vec3d XYZ_n)
{
	return cv::Vec3d(
		XYZ_n[0] * pow(Lab[1]/500. + (Lab[0]+16)/116., 3),
		XYZ_n[1] * pow((Lab[0]+16)/116., 3),
		XYZ_n[2] * pow(-Lab[2]/200. + (Lab[0]+16)/116., 3)
	);
}

double cvutil::Colormetric::deltaE(cv::Vec3d Lab1, cv::Vec3d Lab2)
{
	return cv::norm(Lab1,Lab2, cv::NORM_L2);
}

cv::Vec3d cvutil::Colormetric::cvtXYZ2xyY(cv::Vec3d XYZ)
{
	return cv::Vec3d(
		XYZ[0] / (XYZ[0] + XYZ[1] + XYZ[2]),
		XYZ[1] / (XYZ[0] + XYZ[1] + XYZ[2]),
		XYZ[1]
	);
}

cv::Vec3d cvutil::Colormetric::cvtxyY2XYZ(cv::Vec3d xyY)
{
	return cv::Vec3d(
		xyY[0] / xyY[1] * xyY[2],
		xyY[2],
		(1. - xyY[0] - xyY[1]) / xyY[1] * xyY[2]
	);
}

cv::Vec3d cvutil::Colormetric::cvtDisplayBGR2XYZ(cv::Vec3d BGR, cv::Vec3d gamma, cv::Vec3d Lmax, cv::Vec3d offset, cv::Mat cvtMat)
{
	//	Display Luminance
	cv::Vec3d YBGR;
	for (int i = 0; i < 3; i++) {
		YBGR[i] = Lmax[i] * pow(BGR[i] / 255., gamma[i]) + offset[i];
	}
	//	convert to XYZ
	auto XYZ = cv::Vec3d(cv::Mat(cvtMat*cv::Mat(YBGR)));
	return XYZ;
}

cv::Vec3d cvutil::Colormetric::cvtXYZ2DisplayBGR(cv::Vec3d XYZ, cv::Vec3d gamma, cv::Vec3d Lmax, cv::Vec3d offset, cv::Mat invCvtMat)
{
	//	convert to Display luminance
	auto YBGR = cv::Vec3d(cv::Mat(invCvtMat*cv::Mat(XYZ)));
	//	BGR
	cv::Vec3d BGR;
	for (int i = 0; i < 3; i++) {
		BGR[i] = 255. * pow(YBGR[i] - offset[i], 1. / gamma[i]);
	}
	return BGR;
}

cv::Vec3d cvutil::Colormetric::cvtCamBGR2XYZ(cv::Vec3d BGR, cv::Mat cvtMat)
{
	auto QuadRGB = cvtBGR2QuadRGB(BGR);
	auto XYZ = cv::Mat(cvtMat * cv::Mat(QuadRGB));
	return cv::Vec3d(XYZ);
}

std::vector<double> cvutil::Colormetric::cvtBGR2QuadRGB(cv::Vec3d BGR)
{
	auto c = BGR / 255.0;
	return {
		c[2]*c[2],  // R^2
		c[1]*c[1],  // G^2
		c[0]*c[0],  // B^2
		c[2]*c[1],  // RG
		c[1]*c[0],  // GB
		c[0]*c[2],  // BR
		c[2],       // R
		c[1],       // G
		c[0],       // B
		1.
	};
}

cv::Mat cvutil::Colormetric::cvtMatYBGR2XYZ(std::array<cv::Vec2d, 3> xy)
{
	cv::Mat m = (cv::Mat_<double>(3, 3) << 
		xy[0][0]/xy[0][1], xy[1][0] / xy[1][1], xy[2][0] / xy[2][1],
		1,1,1,
		(1.0 - xy[0][0] - xy[0][1]) / xy[0][1], (1.0 - xy[1][0] - xy[1][1]) / xy[1][1], (1.0 - xy[2][0] - xy[2][1]) / xy[2][1]);
	return m;
}

cv::Mat cvutil::CamColorCalibrator::cvtMat(const double x[])
{
	cv::Mat m(3, 10, CV_64FC1);
	for (int i = 0; i < 30; i++) {
		m.at<double>(i) = x[i];
	}
	return m;
}

cv::Vec3d cvutil::CamColorCalibrator::XYZ_est(const double x[], cv::Vec3d BGR)
{
	return Colormetric::cvtCamBGR2XYZ(BGR, cvtMat(x));
}

cv::Vec3d cvutil::CamColorCalibrator::CIELAB_est(const double x[], cv::Vec3d BGR, cv::Vec3d XYZ_n)
{
	return Colormetric::cvtXYZ2CIELAB(XYZ_est(x, BGR), XYZ_n);
}

void cvutil::CamColorCalibrator::paramInit()
{
	params = cv::Mat::zeros(1, 30, CV_64FC1);
	calibrated = false;
}

double cvutil::CamColorCalibrator::fit(std::vector<cv::Vec3d> BGRs, std::vector<cv::Vec3d> XYZs)
{
	if (BGRs.empty() || XYZs.empty()) return -1;
	if (BGRs.size() != XYZs.size()) return -1;

	// whiteXYZが不正な値の場合は輝度最大点を白色点に指定する
	if (whiteXYZ[0] <= 0 || whiteXYZ[1] <= 0 || whiteXYZ[2] <= 0) {
		cv::Vec3d XYZ_n(0,0,0);
		for (auto XYZ : XYZs) {
			XYZ_n = (XYZ[1] < XYZ_n[1]) ? XYZ_n : XYZ;
		}
		whiteXYZ = XYZ_n;
	}

	// 探索の初期値：線形解法(XYZで最小化した時)の値
	// y = a * param_s.t()
	// param_s.t() = (a.t()*a).inv() * a.t() * y
	cv::Mat y = cv::Mat(XYZs).reshape(1, XYZs.size());
	cv::Mat a(BGRs.size(), 10, CV_64FC1);
	a.forEach<double>([BGRs](double &c, const int p[])->void {
		auto quad = Colormetric::cvtBGR2QuadRGB(BGRs[p[0]]);
		c = quad[p[1]];
	});
	cv::Mat param_s = ((a.t()*a).inv()*a.t()*y).t();

	// パラメータベクトル
	cv::Mat param = param_s.reshape(1, 30);
	// 初期移動量
	cv::Mat step = (cv::Mat_<double>(1, 30) <<
		0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1,
		0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1,
		0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1);

	// 目的関数の設定
	auto ptr_func(new CostFunciton());
	ptr_func->patchBGRs = BGRs;
	ptr_func->patchXYZs = XYZs;
	ptr_func->whitepoint = whiteXYZ;
	// ソルバーの設定
	auto solver = cv::DownhillSolver::create();
	solver->setFunction(ptr_func);
	solver->setInitStep(step);
	solver->setTermCriteria(cv::TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10000, 1e-6));
	// 最小化実行
	double res = solver->minimize(param);

	// パラメータベクトルの保存
	params = param.clone();
	calibrated = true;

	return res;
}

void cvutil::CamColorCalibrator::deltaEs(std::vector<cv::Vec3d> BGRs, std::vector<cv::Vec3d> XYZs, std::vector<double> &dEs)
{
	if (BGRs.empty() || XYZs.empty()) return;
	if (BGRs.size() != XYZs.size()) return;
	if (!calibrated) return;

	dEs.clear();
	for (int i = 0; i < BGRs.size(); i++) {
		double dE = Colormetric::deltaE(
			CIELAB_est(BGRs[i]), 
			Colormetric::cvtXYZ2CIELAB(XYZs[i], whiteXYZ)
		);
		dEs.push_back(dE);
	}
}

void cvutil::CamColorCalibrator::XYZs_est(std::vector<cv::Vec3d> BGRs, std::vector<cv::Vec3d>& XYZs)
{
	if (!calibrated) return;
	if (BGRs.empty())return;

	XYZs.clear();
	for (auto BGR : BGRs) {
		XYZs.push_back(XYZ_est(BGR));
	}
}

cv::Mat cvutil::CamColorCalibrator::cvtMat()
{
	return params.reshape(1, 3);
}

cv::Vec3d cvutil::CamColorCalibrator::XYZ_est(cv::Vec3d BGR)
{
	return Colormetric::cvtCamBGR2XYZ(BGR, cvtMat());
}

cv::Vec3d cvutil::CamColorCalibrator::CIELAB_est(cv::Vec3d BGR)
{
	return Colormetric::cvtXYZ2CIELAB(XYZ_est(BGR), whiteXYZ);
}

void cvutil::CamColorCalibrator::read(cv::String path)
{
	// XMLファイルの読み込み
	cv::FileStorage fs(path, cv::FileStorage::READ);
	fs["param"] >> params;
	fs["white"] >> whiteXYZ;
	
	calibrated = true;
}

void cvutil::CamColorCalibrator::write(cv::String path)
{
	// XMLファイルへの書き出し
	cv::FileStorage fs(path, cv::FileStorage::WRITE);
	fs << "param" << params;
	fs << "white" << whiteXYZ;
}

double cvutil::CamColorCalibrator::CostFunciton::calc(const double * x) const
{
	// パッチすべての色についてLab色空間内で二乗誤差を最小化
	std::vector<cv::Vec3d> error;
	for (int i = 0; i < patchBGRs.size(); i++) {
		auto BGR = patchBGRs[i];
		auto XYZ = patchXYZs[i];
		auto Lab_est = CIELAB_est(x, BGR, whitepoint);
		auto Lab_msr = Colormetric::cvtXYZ2CIELAB(XYZ, whitepoint);
		error.push_back(Lab_est - Lab_msr);
	}
	// 誤差二乗和を計算
	double cost = 0.0;
	for (auto e : error) {
		cost += e.dot(e);
	}
	return cost;
}

double cvutil::DisplayColorCalibrator::fit(std::vector<double> BGR_vals, std::array<std::vector<cv::Vec3d>, 3> xyLs)
{
	//	step 1: xyL -> XYZ, XYZバックライト差分, 差分のxyL
	std::array<std::vector<cv::Vec3d>, 3> XYZs, XYZs_sub, xyLs_sub;
	offset = Colormetric::cvtxyY2XYZ(xyLs[0][0]);
	for (auto i = 0; i < 3; i++) {
		for (auto xyL : xyLs[i]) {
			auto XYZ = Colormetric::cvtxyY2XYZ(xyL);
			auto XYZ_sub = XYZ - offset;
			auto xyL_sub = Colormetric::cvtXYZ2xyY(XYZ_sub);

			XYZs[i].push_back(XYZ);
			XYZs_sub[i].push_back(XYZ - offset);
			xyLs_sub[i].push_back(xyL_sub);
		}
		//	(B,G,R)=(0,0,0)の部分を削除
		XYZs_sub[i].erase(XYZs_sub[i].begin());
		xyLs_sub[i].erase(xyLs_sub[i].begin());

		//	step 2: バックライト差分の平均色度を求める
		cv::Vec2d xy_mean(0,0);
		for (auto xyL : xyLs_sub[i]) {
			xy_mean += cv::Vec2d(xyL[0],xyL[1]);
		}
		xy_bgr[i] = xy_mean;
	}

	//	BGR -> bgr
	std::vector<double> bgrs;
	for (auto BGR: BGR_vals) {
		bgrs.push_back(BGR / 255.0);
	}
	bgrs.erase(bgrs.begin());

	//	step 3: 対数をとって線形最小二乗法で解く
	//		L = L_max * bgr^gamma
	//		->  ln(L) = ln(L_max) + gamma*ln(bgr)
	//		->  [y] = [x,1] * [L_max, gamma]^t
	for (auto i = 0; i < 3; i++) {
		cv::Mat my(bgrs.size(), 1, CV_64FC1);
		cv::Mat mx(bgrs.size(), 2, CV_64FC1);
		cv::Mat ma;
		//	代入
		for (int j = 0; j < bgrs.size(); j++) {
			auto L = xyLs_sub[i][j][3];
			my.at<double>(j) = std::log(std::max(L, 1e-4));
			mx.at<double>(j, 0) = std::log(bgrs[j]);
			mx.at<double>(j, 1) = 1.0;
		}
		//	線形最小二乗法
		cv::solve(mx, my, ma, cv::DECOMP_SVD);
		Lmax[i] = ma.at<double>(0);
		gamma[i] = ma.at<double>(1);
	}

	return 0.0;
}

void cvutil::DisplayColorCalibrator::XYZs_est(std::vector<cv::Vec3d> BGRs, std::vector<cv::Vec3d>& XYZs)
{
	auto M = cvtMat();
	for (auto BGR : BGRs) {
		auto XYZ = Colormetric::cvtDisplayBGR2XYZ(BGR, gamma, Lmax, offset, M);
		XYZs.push_back(XYZ);
	}
}
