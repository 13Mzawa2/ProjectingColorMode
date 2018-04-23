
# include <Siv3D.hpp>
#include "TestAreas.h"

void SetGUI(GUI &gui)
{
	gui.style.borderRadius = 0.0;
	gui.style.borderColor = Palette::Darkgray;
	
	gui.setTitle(L"Settings");
	gui.addln(L"ColorSwitch", GUIRadioButton::Create({L"Test", L"Background"}, 0u, true));
	gui.addln(L"BackgroundText", GUIText::Create(L"Background: (0.5,0.5,0.5)", 300));
	gui.addln(L"TestText", GUIText::Create(L"Test: (0.5,0.5,0.5)", 300));
	gui.addln(L"ControllerState", GUIText::Create(L"Controller: none"));
}

void updateColor(ColorF &c, XInput controller)
{
	double k = (controller.buttonA.pressed) ? 0.01 : 0.001;
	double dY =
		(c.r >= 1 || c.g >= 1 || c.b >= 1) ?
		-k * controller.leftTrigger :
		(c.r <= 0.001 || c.g <= 0.001 || c.b <= 0.001) ?
		k * controller.rightTrigger :
		k*(controller.rightTrigger - controller.leftTrigger);
	c.r += dY*c.r;
	c.g += dY*c.g;
	c.b += dY*c.b;

}

void Main()
{
	//	ウィンドウサイズの設定
	//	プロジェクタ最大サイズ＋メインモニタにはみ出す量
	const int winOffset = 800;
	const Size proSize(1024, 768);
	const Size monitorSize(1920, 1080);
	const Size winSize(proSize.x+winOffset, proSize.y);
	const Point winPos(monitorSize.x-winOffset, 0);
	const Point proPos(winOffset, 0);

	Window::SetStyle(WindowStyle::NonFrame);
	Window::Resize(winSize);
	Window::SetPos(winPos);
	Graphics::SetBackground(ColorF(0.5,0.5,0.5));

	//	入力インターフェース
	GUI gui(GUIStyle::Default);
	SetGUI(gui);
	GUI palette_gui(GUIStyle::Default);
	palette_gui.style.borderRadius = 0.0;
	palette_gui.setPos(300, 0);
	palette_gui.setTitle(L"Palette");
	palette_gui.add(L"ColorPalette", GUIColorPalette::Create(ColorF(0.5, 0.5, 0.5)));

	//	テスト刺激
	//	時計回りに指定する
	std::vector<Quad> testAreas;
	TestAreas ts(proPos + Point(500, 370), Size(72, 72));
	//	背景
	ColorF background(0.5, 0.5, 0.5);

	//	iniファイルがあれば読込
	INIReader inir(L"data.ini");
	if (inir) {
		std::vector<Vec2> corners{
			inir.get<Vec2>(L"Corners.c0"),
			inir.get<Vec2>(L"Corners.c1"),
			inir.get<Vec2>(L"Corners.c2"),
			inir.get<Vec2>(L"Corners.c3")
		};
		ts.color = inir.get<ColorF>(L"Color.test");
		background = inir.get<ColorF>(L"Color.back");
		ts.init(corners);
	}

	while (System::Update())
	{
		auto controller = XInput(0);

		if (controller.isConnected()) {
			gui.text(L"ControllerState").text = L"Controller: connected";
			//	コントローラ操作での色変化
			if (gui.radioButton(L"ColorSwitch").checked(0u)) {
				updateColor(ts.color, controller);
			}
			else {
				updateColor(background, controller);
			}
			//	トグルスイッチ切り替え
			if (controller.buttonY.clicked) {
				gui.radioButton(L"ColorSwitch").checked(1u);
			}
			if (controller.buttonX.clicked) {
				gui.radioButton(L"ColorSwitch").checked(0u);
			}
		}

		//	ColorPaletteでの色変化
		if (palette_gui.colorPalette(L"ColorPalette").pressed) {
			if (gui.radioButton(L"ColorSwitch").checked(0u)) {
				ts.color = palette_gui.colorPalette(L"ColorPalette").colorF;
			}
			else {
				background = palette_gui.colorPalette(L"ColorPalette").colorF;
			}
		}
		//	表示の変化
		gui.text(L"TestText").text =
			L"Test: (" + Format(DecimalPlace(3), ts.color.r)
			+ L"," + Format(DecimalPlace(3), ts.color.g)
			+ L"," + Format(DecimalPlace(3), ts.color.b)
			+ L")";
		gui.text(L"BackgroundText").text =
			L"Background: (" + Format(DecimalPlace(3), background.r)
			+ L"," + Format(DecimalPlace(3), background.g) 
			+ L"," + Format(DecimalPlace(3), background.b)
			+ L")";

		//	刺激の作成
		ts.update();
		ts.draw();
		Graphics::SetBackground(background);
	}
	//	終了時に現在の設定をiniファイルに保存
	INIWriter ini(L"data.ini");
	ini.write(L"Corners", L"c0", ts.corners[0]);
	ini.write(L"Corners", L"c1", ts.corners[1]);
	ini.write(L"Corners", L"c2", ts.corners[2]);
	ini.write(L"Corners", L"c3", ts.corners[3]);
	ini.write(L"Color", L"back", background);
	ini.write(L"Color", L"test", ts.color);
}
