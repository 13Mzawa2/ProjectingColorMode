#pragma once

#include <Siv3D.hpp>

class GrabQuad
{
private:
	std::vector<Quad> quads;
	std::vector<bool> grabbed = { false, false, false, false };

public:
	std::vector<Vec2> corners;
	ColorF color = ColorF(0.5, 0.5, 0.5);

	GrabQuad() = default;
	explicit GrabQuad(Point p, Size sz) {
		init(p, sz);
	}
	void init(Point p, Size sz)
	{
		corners = std::vector<Vec2>{ p, p + Vec2{ sz.x, 0 }, p + sz, p + Vec2{ 0,sz.y } };
		makeQuads();
	}
	void init(std::vector<Vec2> corners_)
	{
		corners = corners_;
		makeQuads();
	}
	void makeQuads()
	{
		Vec2 center = (corners[0] + corners[1] + corners[2] + corners[3]) / 4;
		Quad lu(
			corners[0],
			corners[0] + (corners[1] - corners[0]) / 2,
			center,
			corners[0] + (corners[3] - corners[0]) / 2
		);
		Quad ru(
			corners[1] + (corners[0] - corners[1]) / 2,
			corners[1],
			corners[1] + (corners[2] - corners[1]) / 2,
			center
		);
		Quad rb(
			center,
			corners[2] + (corners[1] - corners[2]) / 2,
			corners[2],
			corners[2] + (corners[3] - corners[2]) / 2
		);
		Quad lb(
			corners[3] + (corners[0] - corners[3]) / 2,
			center,
			corners[3] + (corners[2] - corners[3]) / 2,
			corners[3]
		);

		quads.clear();
		quads = std::vector<Quad>{ lu, ru, rb, lb };
	}
	void update()
	{
		for (auto i = 0; i < 4; i++) {
			if (quads[i].leftClicked) {
				grabbed[i] = true;
			}
			else if (Input::MouseL.released) {
				grabbed[i] = false;
			}
			else if (grabbed[i]) {
				corners[i] += Mouse::Delta();
			}
		}
		makeQuads();
	}
	void draw() const
	{
		for (auto q : quads) {
			if (q.mouseOver) q.draw(Palette::Yellow);
			else q.draw(color);
			//else q.draw();
		}
	}
};