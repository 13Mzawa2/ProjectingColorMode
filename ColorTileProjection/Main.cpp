#include <Siv3DAddon\OpenCV.hpp>
# include <Siv3D.hpp>
#include "GrabQuad.h"
#include "TileTexture.h"
#include <codecvt>

//	Window parameters
const Size proSize(1280, 800);
const Size monitorSize(1920, 1080);
const int menuWidth = 800;
const Size winSize(proSize.x + menuWidth, proSize.y);
const Point winPos(monitorSize.x - menuWidth, 0);
const Point proPos(menuWidth, 0);

String inifile = L"data.ini";

void readCalibCSV(std::vector<double> &BGR_vals, std::array<std::vector<cv::Vec3d>, 3> &bgr_xyLs)
{
	if (const auto open = Dialog::GetOpen({ ExtensionFilter::CSV })) {
		CSVReader csv(open.value());
		if (csv.isOpened()) {
			BGR_vals.clear();
			for (int j = 0; j <= csv.rows - 3; j++) {
				BGR_vals.push_back(std::floor(j * 255.0 / double(csv.rows - 3)));
			}
			for (int i = 0; i < 3; i++) {
				bgr_xyLs[i].clear();
				for (int j = 0; j <= csv.rows - 3; j++) {
					cv::Vec3d xyL(
						csv.get<double>(j, (2 - i) * 3 + 0),
						csv.get<double>(j, (2 - i) * 3 + 1),
						csv.get<double>(j, (2 - i) * 3 + 2));
					bgr_xyLs[i].push_back(xyL);
				}
			}

		}
	}
}

void makeMenu(GUI &gui)
{
	gui.setTitle(L"main menu");
	gui.addln(L"calib_button", GUIButton::Create(L"Load projector calib data"));
	gui.add(L"calib_read_button", GUIButton::Create(L"Load calib xml"));
	gui.addln(L"calib_write_button", GUIButton::Create(L"Save calib xml"));
}

void Main()
{
	//	Window setting
	Window::SetStyle(WindowStyle::NonFrame);
	Window::Resize(winSize);
	Window::SetPos(winPos);
	Graphics::SetBackground(ColorF(0.5, 0.5, 0.5));

	//	making gui
	GUI gui(GUIStyle::Default);
	makeMenu(gui);

	//	initial content
	TileTexture tile;
	Rect prolight(proPos, proSize);
	GrabQuad gq(proPos + Point(200, 200), Size(300, 300));

	//	read ini file if it exists
	INIReader inir(inifile);
	if (inir) {
		std::vector<Vec2> corners{
			inir.get<Vec2>(L"Corners.c0"),
			inir.get<Vec2>(L"Corners.c1"),
			inir.get<Vec2>(L"Corners.c2"),
			inir.get<Vec2>(L"Corners.c3")
		};
		gq.init(corners);
	}

	while (System::Update())
	{
		//	gui events
		if (gui.button(L"calib_button").pushed) {
			readCalibCSV(tile.BGR_vals, tile.bgr_xyLs);
			tile.calib.fit(tile.BGR_vals, tile.bgr_xyLs);
		}
		if (gui.button(L"calib_read_button").pushed) {
			if (const auto open = Dialog::GetOpen({ ExtensionFilter::XML })) {
				std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
				auto str = converter.to_bytes(open.value().c_str());
				tile.calib.read(str);
			}
		}
		if (gui.button(L"calib_write_button").pushed) {
			if (const auto save = Dialog::GetSave({ ExtensionFilter::XML })) {
				std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
				auto str = converter.to_bytes(save.value().c_str());
				tile.calib.write(str);
			}
		}

		//	update contents
		gq.update();
		Quad quad(gq.corners[0], gq.corners[1], gq.corners[2], gq.corners[3]);
		//	draw contents
		prolight.draw(ColorF(1., 1., 1.));
		quad.draw(Palette::Lime);

	}

	//	save position to ini file
	INIWriter iniw(inifile);
	iniw.write(L"Corners", L"c0", gq.corners[0]);
	iniw.write(L"Corners", L"c1", gq.corners[1]);
	iniw.write(L"Corners", L"c2", gq.corners[2]);
	iniw.write(L"Corners", L"c3", gq.corners[3]);
}
