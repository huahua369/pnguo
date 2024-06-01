

struct PassParameters
{
	PassParameters()
		: uImageWidth(1)
		, uImageHeight(1)
		, iMousePos{ 0, 0 }
		, fBorderColorRGB{ 1, 1, 1, 1 }
		, fMagnificationAmount(6.0f)
		, fMagnifierScreenRadius(0.35f)
		, iMagnifierOffset{ 500, -500 }
	{}

	uint32_t    uImageWidth;
	uint32_t    uImageHeight;
	int         iMousePos[2];            // in pixels, driven by ImGuiIO.MousePos.xy

	float       fBorderColorRGB[4];      // Linear RGBA

	float       fMagnificationAmount;    // [1-...]
	float       fMagnifierScreenRadius;  // [0-1]
	mutable int iMagnifierOffset[2];     // in pixels
};
