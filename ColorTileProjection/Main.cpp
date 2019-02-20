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
cv::String texpath = "tilesrc.xml";

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
	gui.addln(L"tex_enable", GUIToggleSwitch::Create(L"Texture OFF", L"ON", false));
}

void makeEditMenu(GUI &gui)
{
	gui.setPos(0, 200);
	gui.setTitle(L"edit menu");
	gui.add(L"radio_col1", GUIRadioButton::Create({ L"11",L"21",L"31",L"41",L"51",L"61" }, 0u));
	gui.add(L"radio_col2", GUIRadioButton::Create({ L"12",L"22",L"32",L"42",L"52",L"62" }, none));
	gui.addln(L"radio_col3", GUIRadioButton::Create({ L"13",L"23",L"33",L"43",L"53",L"63" }, none));
	gui.add(L"slider_x", GUISlider::Create(-0.03, 0.03, 0));
	gui.addln(L"x_field", GUITextField::Create(none));
	gui.add(L"slider_y", GUISlider::Create(-0.03, 0.03, 0));
	gui.addln(L"y_field", GUITextField::Create(none));
	gui.add(L"slider_L", GUISlider::Create(-400, 400, 0));
	gui.addln(L"L_field", GUITextField::Create(none));
	gui.addln(L"gauss_slider", GUISlider::Create(0.0, 10.0, 0.0));
	gui.addln(L"gauss_radio", GUIRadioButton::Create({ L"0", L"1.0", L"2.0", L"5.0", L"slider"}, 4u));
	gui.addln(L"sw_invert", GUIToggleSwitch::Create(L"Default", L"Inverted", false));
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
	GUI tex_gui(GUIStyle::Default);
	makeEditMenu(tex_gui);

	//	initial content
	TileTexture tile;
	cv::Mat img_cv;
	Rect prolight(proPos, proSize);
	GrabQuad gq(proPos + Point(200, 200), Size(300, 300));
	DynamicTexture tex;

	//	texture
	cv::Mat tilecolor, tilecolor_modulate;
	//	initialization
	tilecolor = (cv::Mat_<cv::Vec3d>(6, 3) <<
		cv::Vec3d(0.33, 0.33, 100.0), cv::Vec3d(0.33, 0.33, 100.0), cv::Vec3d(0.33, 0.33, 100.0),
		cv::Vec3d(0.33, 0.33, 100.0), cv::Vec3d(0.33, 0.33, 100.0), cv::Vec3d(0.33, 0.33, 100.0),
		cv::Vec3d(0.33, 0.33, 100.0), cv::Vec3d(0.33, 0.33, 100.0), cv::Vec3d(0.33, 0.33, 100.0),
		cv::Vec3d(0.33, 0.33, 100.0), cv::Vec3d(0.33, 0.33, 100.0), cv::Vec3d(0.33, 0.33, 100.0),
		cv::Vec3d(0.33, 0.33, 100.0), cv::Vec3d(0.33, 0.33, 100.0), cv::Vec3d(0.33, 0.33, 100.0),
		cv::Vec3d(0.33, 0.33, 100.0), cv::Vec3d(0.33, 0.33, 100.0), cv::Vec3d(0.33, 0.33, 100.0));
	tilecolor_modulate = cv::Mat::zeros(6, 3, CV_64FC3);
	cv::FileStorage fs(texpath, cv::FileStorage::READ);
	if (fs.isOpened()) {
		fs["tilecolor"] >> tilecolor;
		fs["difference"] >> tilecolor_modulate;
	}

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
		if (gui.toggleSwitch(L"tex_enable").isRight) {
			if (tile.calib.calibrated) {

				double val=0;
				//	set gaussian
				switch (tex_gui.radioButton(L"gauss_radio").checkedItem.value()) {
				case 0u:
					val = 0.0;
					break;
				case 1u:
					val = 1.0;
					break;
				case 2u:
					val = 2.5;
					break;
				case 3u:
					val = 8.0;
					break;
				default:
					val = tex_gui.slider(L"gauss_slider").value;
					break;
				}
				img_cv = tile.makeTileTexture(
					tilecolor + tilecolor_modulate, 
					cv::Size(1024, 1024), cv::Size(21,21),
					val, tex_gui.toggleSwitch(L"sw_invert").isRight);

				std::vector<cv::Point> corners{
					cv::Point(gq.corners[0].x - menuWidth,gq.corners[0].y),
					cv::Point(gq.corners[1].x - menuWidth,gq.corners[1].y),
					cv::Point(gq.corners[2].x - menuWidth,gq.corners[2].y),
					cv::Point(gq.corners[3].x - menuWidth,gq.corners[3].y)
				};
				auto temp = tile.transformedTexture(img_cv, cv::Size(proSize.x, proSize.y), corners);
				cv::cvtColor(temp, temp, cv::COLOR_BGR2RGB);
				Image img = OpenCV::ToImage(cv::Mat_<cv::Vec3b>(temp));
				tex.tryFill(img);
			}
		}

		//	exclusive-or
		//	current checked item
		static int checked_col = 0;
		static int checked_row = 0;
		if (tex_gui.radioButton(L"radio_col1").hasChanged) {
			tex_gui.radioButton(L"radio_col2").check(none);
			tex_gui.radioButton(L"radio_col3").check(none);
			checked_col = 0;
			checked_row = tex_gui.radioButton(L"radio_col" + ToString(checked_col+1)).checkedItem.value();
			//	fill xyL value
			tex_gui.textField(L"x_field").setText(ToString(tilecolor.at<cv::Vec3d>(checked_row, checked_col)[0]));
			tex_gui.textField(L"y_field").setText(ToString(tilecolor.at<cv::Vec3d>(checked_row, checked_col)[1]));
			tex_gui.textField(L"L_field").setText(ToString(tilecolor.at<cv::Vec3d>(checked_row, checked_col)[2]));
			//	fill diff value
			tex_gui.slider(L"slider_x").setValue(tilecolor_modulate.at<cv::Vec3d>(checked_row, checked_col)[0]);
			tex_gui.slider(L"slider_y").setValue(tilecolor_modulate.at<cv::Vec3d>(checked_row, checked_col)[1]);
			tex_gui.slider(L"slider_L").setValue(tilecolor_modulate.at<cv::Vec3d>(checked_row, checked_col)[2]);
		}
		if (tex_gui.radioButton(L"radio_col2").hasChanged) {
			tex_gui.radioButton(L"radio_col3").check(none);
			tex_gui.radioButton(L"radio_col1").check(none);
			checked_col = 1;
			checked_row = tex_gui.radioButton(L"radio_col" + ToString(checked_col+1)).checkedItem.value();
			//	fill xyL value
			tex_gui.textField(L"x_field").setText(ToString(tilecolor.at<cv::Vec3d>(checked_row, checked_col)[0]));
			tex_gui.textField(L"y_field").setText(ToString(tilecolor.at<cv::Vec3d>(checked_row, checked_col)[1]));
			tex_gui.textField(L"L_field").setText(ToString(tilecolor.at<cv::Vec3d>(checked_row, checked_col)[2]));
			//	fill diff value
			tex_gui.slider(L"slider_x").setValue(tilecolor_modulate.at<cv::Vec3d>(checked_row, checked_col)[0]);
			tex_gui.slider(L"slider_y").setValue(tilecolor_modulate.at<cv::Vec3d>(checked_row, checked_col)[1]);
			tex_gui.slider(L"slider_L").setValue(tilecolor_modulate.at<cv::Vec3d>(checked_row, checked_col)[2]);
		}
		if (tex_gui.radioButton(L"radio_col3").hasChanged) {
			tex_gui.radioButton(L"radio_col1").check(none);
			tex_gui.radioButton(L"radio_col2").check(none);
			checked_col = 2;
			checked_row = tex_gui.radioButton(L"radio_col" + ToString(checked_col+1)).checkedItem.value();
			//	fill xyL value
			tex_gui.textField(L"x_field").setText(ToString(tilecolor.at<cv::Vec3d>(checked_row, checked_col)[0]));
			tex_gui.textField(L"y_field").setText(ToString(tilecolor.at<cv::Vec3d>(checked_row, checked_col)[1]));
			tex_gui.textField(L"L_field").setText(ToString(tilecolor.at<cv::Vec3d>(checked_row, checked_col)[2]));
			//	fill diff value
			tex_gui.slider(L"slider_x").setValue(tilecolor_modulate.at<cv::Vec3d>(checked_row, checked_col)[0]);
			tex_gui.slider(L"slider_y").setValue(tilecolor_modulate.at<cv::Vec3d>(checked_row, checked_col)[1]);
			tex_gui.slider(L"slider_L").setValue(tilecolor_modulate.at<cv::Vec3d>(checked_row, checked_col)[2]);
		}
		//	slider has changed
		if (tex_gui.slider(L"slider_x").hasChanged) {
			//	change x diff val
			tilecolor_modulate.at<cv::Vec3d>(checked_row, checked_col)[0] = tex_gui.slider(L"slider_x").value;
		}
		if (tex_gui.slider(L"slider_y").hasChanged) {
			//	change y diff val
			tilecolor_modulate.at<cv::Vec3d>(checked_row, checked_col)[1] = tex_gui.slider(L"slider_y").value;
		}
		if (tex_gui.slider(L"slider_L").hasChanged) {
			//	change L diff val
			tilecolor_modulate.at<cv::Vec3d>(checked_row, checked_col)[2] = tex_gui.slider(L"slider_L").value;
		}
		//	text-field has changed
		if (tex_gui.textField(L"x_field").hasChanged) {
			//	change x val
			tilecolor.at<cv::Vec3d>(checked_row, checked_col)[0] = FromString<double>(tex_gui.textField(L"x_field").text);
		}
		if (tex_gui.textField(L"y_field").hasChanged) {
			//	change y val
			tilecolor.at<cv::Vec3d>(checked_row, checked_col)[1] = FromString<double>(tex_gui.textField(L"y_field").text);
		}
		if (tex_gui.textField(L"L_field").hasChanged) {
			//	change L val
			tilecolor.at<cv::Vec3d>(checked_row, checked_col)[2] = FromString<double>(tex_gui.textField(L"L_field").text);
		}


		//	update contents
		gq.update();
		Quad quad(gq.corners[0], gq.corners[1], gq.corners[2], gq.corners[3]);


		//	draw contents
		if (gui.toggleSwitch(L"tex_enable").isLeft) {
			prolight.draw(ColorF(1., 1., 1.));
			quad.draw(Palette::White);	//	debug時には色を付ける
		}
		else {
			prolight(tex).draw();
			//quad.drawFrame(2.0, Palette::White);
		}

	}

	//	save position to ini file
	INIWriter iniw(inifile);
	iniw.write(L"Corners", L"c0", gq.corners[0]);
	iniw.write(L"Corners", L"c1", gq.corners[1]);
	iniw.write(L"Corners", L"c2", gq.corners[2]);
	iniw.write(L"Corners", L"c3", gq.corners[3]);

	//	save tile color
	cv::FileStorage fs_w(texpath, cv::FileStorage::WRITE);
	fs_w << "tilecolor" << tilecolor;
	fs_w << "difference" << tilecolor_modulate;
	fs_w.release();
}
