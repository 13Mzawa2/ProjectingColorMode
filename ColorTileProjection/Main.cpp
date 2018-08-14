#include <Siv3DAddon\OpenCV.hpp>
# include <Siv3D.hpp>
#include "GrabQuad.h"
#include "TileTexture.h"

//	Window parameters
const Size proSize(1280, 800);
const Size monitorSize(1920, 1080);
const int menuWidth = 800;
const Size winSize(proSize.x + menuWidth, proSize.y);
const Point winPos(monitorSize.x - menuWidth, 0);
const Point proPos(menuWidth, 0);

String inifile = L"data.ini";

void readCalibCSV()
{

}

void makeMenu(GUI &gui)
{
	gui.setTitle(L"main menu");
	gui.addln(L"calib_button", GUIButton::Create(L"Load projector calib data"));

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
