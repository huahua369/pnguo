// AMD SampleVK sample code
// 
// Copyright(c) 2020 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stdafx.h"

#include "UI.h"
#include "GLTFSample.h"
#include "imgui.h"

#include "base/FrameworkWindows.h"

// To use the 'disabled UI state' functionality (ImGuiItemFlags_Disabled), include internal header
// https://github.com/ocornut/imgui/issues/211#issuecomment-339241929
#include "imgui_internal.h"
static void DisableUIStateBegin(const bool& bEnable)
{
	if (!bEnable)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}
};
static void DisableUIStateEnd(const bool& bEnable)
{
	if (!bEnable)
	{
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	}
};

// Some constants and utility functions
static constexpr float MAGNIFICATION_AMOUNT_MIN = 1.0f;
static constexpr float MAGNIFICATION_AMOUNT_MAX = 32.0f;
static constexpr float MAGNIFIER_RADIUS_MIN = 0.01f;
static constexpr float MAGNIFIER_RADIUS_MAX = 0.85f;
static constexpr float MAGNIFIER_BORDER_COLOR__LOCKED[3] = { 0.002f, 0.72f, 0.0f }; // G
static constexpr float MAGNIFIER_BORDER_COLOR__FREE[3] = { 0.72f, 0.002f, 0.0f }; // R
template<class T> static T clamped(const T& v, const T& min, const T& max)
{
	if (v < min)      return min;
	else if (v > max) return max;
	else              return v;
}

void DrawGrid(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& matrix, const float gridSize, glm::vec4 rc);


void GLTFSample::BuildUI()
{
	// if we haven't initialized GLTFLoader yet, don't draw UI.
	if (m_pGltfLoader == nullptr)
	{
		LoadScene(m_activeScene);
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;

	const uint32_t W = this->GetWidth();
	const uint32_t H = this->GetHeight();

	const uint32_t PROFILER_WINDOW_PADDIG_X = 10;
	const uint32_t PROFILER_WINDOW_PADDIG_Y = 10;
	const uint32_t PROFILER_WINDOW_SIZE_X = 330;
	const uint32_t PROFILER_WINDOW_SIZE_Y = 450;
	const uint32_t PROFILER_WINDOW_POS_X = W - PROFILER_WINDOW_PADDIG_X - PROFILER_WINDOW_SIZE_X;
	const uint32_t PROFILER_WINDOW_POS_Y = PROFILER_WINDOW_PADDIG_Y;

	const uint32_t CONTROLS_WINDOW_POS_X = 10;
	const uint32_t CONTROLS_WINDOW_POS_Y = 10;
	const uint32_t CONTROLW_WINDOW_SIZE_X = 350;
	const uint32_t CONTROLW_WINDOW_SIZE_Y = 780; // assuming > 720p

	// Render CONTROLS window
	auto ps = ImVec2(CONTROLS_WINDOW_POS_X, CONTROLS_WINDOW_POS_Y);
	auto ss = ImVec2(CONTROLW_WINDOW_SIZE_X, CONTROLW_WINDOW_SIZE_Y);
	//ImGui::SetNextWindowPos(ps, ImGuiCond_FirstUseEver);
	//ImGui::SetNextWindowSize({W,H}, ImGuiCond_FirstUseEver);

	glm::vec4 rc = { 0,0,W, H };
	glm::mat4 cameraView(1.0), cameraProjection(1.0);

	auto cam = m_camera;
	//Sets the camera
	auto mCameraCurrViewProj = cam.GetProjection() * cam.GetView();
	auto mCameraPrevViewProj = cam.GetProjection() * cam.GetPrevView();
	// more accurate calculation
	auto mInverseCameraCurrViewProj = glm::affineInverse(cam.GetView()) * glm::inverse(cam.GetProjection());
	auto cameraPos = cam.GetPosition();
	auto ss1 = ImVec2(W, H);
	ps.x = 0; ps.y = 0;
	//ImGui::Begin("grid", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse);
	//ImGui::SetWindowPos(ps, ImGuiCond_FirstUseEver);
	//ImGui::SetWindowSize(ss1, ImGuiCond_FirstUseEver);
	//DrawGrid(cam.GetView(), cam.GetProjection(), glm::mat4(1.0), 100.f, rc);
	//ImGui::End();
	if (m_UIState.bShowControlsWindow)
	{
		ImGui::Begin("CONTROLS (F1)", &m_UIState.bShowControlsWindow);
		if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Play", &m_bPlay);
			ImGui::SliderFloat("Time", &m_time, 0, 30);
		}

		ImGui::Spacing();
		ImGui::Spacing();

		if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen))
		{
			const char* cameraControl[] = { "Orbit", "WASD", "cam #0", "cam #1", "cam #2", "cam #3" , "cam #4", "cam #5" };

			if (m_activeCamera >= m_pGltfLoader->m_cameras.size() + 2)
				m_activeCamera = 0;
			ImGui::Combo("Camera", &m_activeCamera, cameraControl, std::min((m_pGltfLoader->m_cameras.size() + 2), _countof(cameraControl)));

			auto getterLambda = [](void* data, int idx, const char** out_str)->bool { *out_str = ((std::vector<std::string> *)data)->at(idx).c_str(); return true; };
			if (ImGui::Combo("Model", &m_activeScene, getterLambda, &m_sceneNames, (int)m_sceneNames.size()))
			{
				LoadScene(m_activeScene);

				//bail out as we need to reload everything
				ImGui::End();
				ImGui::EndFrame();
				ImGui::NewFrame();
				return;
			}

			ImGui::SliderFloat("Emissive Intensity", &m_UIState.EmissiveFactor, 1.0f, 1000.0f, NULL, 1.0f);

			const char* skyDomeType[] = { "Procedural Sky", "cubemap", "Simple clear" };
			ImGui::Combo("Skydome", &m_UIState.SelectedSkydomeTypeIndex, skyDomeType, _countof(skyDomeType));

			ImGui::SliderFloat("IBL Factor", &m_UIState.IBLFactor, 0.0f, 3.0f);
			for (int i = 0; i < m_pGltfLoader->m_lights.size(); i++)
			{
				ImGui::SliderFloat(format("Light %i Intensity", i).c_str(), &m_pGltfLoader->m_lights[i].m_intensity, 0.0f, 50.0f);
			}
			if (ImGui::Button("Set Spot Light 0 to Camera's View"))
			{
				int idx = m_pGltfLoader->m_lightInstances[0].m_nodeIndex;
				auto target = m_camera.GetPosition() - m_camera.GetDirection();
				auto source = m_camera.GetPosition();
				m_pGltfLoader->m_nodes[idx].m_tranform.LookAt(source, target, false);
				m_pGltfLoader->m_animatedMats[idx] = m_pGltfLoader->m_nodes[idx].m_tranform.GetWorldMat();
			}
		}

		ImGui::Spacing();
		ImGui::Spacing();

		if (ImGui::CollapsingHeader("PostProcessing", ImGuiTreeNodeFlags_DefaultOpen))
		{
			const char* tonemappers[] = { "AMD Tonemapper", "DX11DSK", "Reinhard", "Uncharted2Tonemap", "ACES", "No tonemapper" };
			ImGui::Combo("Tonemapper", &m_UIState.SelectedTonemapperIndex, tonemappers, _countof(tonemappers));

			ImGui::SliderFloat("Exposure", &m_UIState.Exposure, 0.0f, 4.0f);

			ImGui::Checkbox("TAA", &m_UIState.bUseTAA);
		}

		ImGui::Spacing();
		ImGui::Spacing();

		if (ImGui::CollapsingHeader("Magnifier", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// read in Magnifier pass parameters from the UI & app state
		   // MagnifierPS::PassParameters& params = m_UIState.MagnifierParams;
			PassParameters& params = m_UIState.MagnifierParams;
			params.uImageHeight = m_Height;
			params.uImageWidth = m_Width;
			params.iMousePos[0] = m_UIState.bLockMagnifierPosition ? m_UIState.LockedMagnifiedScreenPositionX : static_cast<int>(io.MousePos.x);
			params.iMousePos[1] = m_UIState.bLockMagnifierPosition ? m_UIState.LockedMagnifiedScreenPositionY : static_cast<int>(io.MousePos.y);

			if (ImGui::Checkbox("Show Magnifier (M)", &m_UIState.bUseMagnifier))
			{
				// We need to update IMGUI's renderpass to draw to magnfier's renderpass when in hdr
				// Hence, flush GPU and update it through OnUpdateDisplay
				// Which needs to do the same thing when display mode is changed.
				m_device.GPUFlush();
				OnUpdateDisplay();
			}

			DisableUIStateBegin(m_UIState.bUseMagnifier);
			{
				// use a local bool state here to track locked state through the UI widget,
				// and then call ToggleMagnifierLockedState() to update the persistent state (m_UIState).
				// the keyboard input for toggling lock directly operates on the persistent state.
				const bool bIsMagnifierCurrentlyLocked = m_UIState.bLockMagnifierPosition;
				bool bMagnifierToggle = bIsMagnifierCurrentlyLocked;
				ImGui::Checkbox("Lock Position (L)", &bMagnifierToggle);

				if (bMagnifierToggle != bIsMagnifierCurrentlyLocked)
					m_UIState.ToggleMagnifierLock();

				ImGui::SliderFloat("Screen Size", &params.fMagnifierScreenRadius, MAGNIFIER_RADIUS_MIN, MAGNIFIER_RADIUS_MAX);
				ImGui::SliderFloat("Magnification", &params.fMagnificationAmount, MAGNIFICATION_AMOUNT_MIN, MAGNIFICATION_AMOUNT_MAX);
				ImGui::SliderInt("OffsetX", &params.iMagnifierOffset[0], -m_Width, m_Width);
				ImGui::SliderInt("OffsetY", &params.iMagnifierOffset[1], -m_Height, m_Height);
			}
			DisableUIStateEnd(m_UIState.bUseMagnifier);
		}

		ImGui::Spacing();
		ImGui::Spacing();

		if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Show Bounding Boxes", &m_UIState.bDrawBoundingBoxes);
			ImGui::Checkbox("Show Light Frustum", &m_UIState.bDrawLightFrustum);

			ImGui::Text("Wireframe");
			ImGui::SameLine(); ImGui::RadioButton("Off", (int*)&m_UIState.WireframeMode, (int)UIState::WireframeMode::WIREFRAME_MODE_OFF);
			ImGui::SameLine(); ImGui::RadioButton("Shaded", (int*)&m_UIState.WireframeMode, (int)UIState::WireframeMode::WIREFRAME_MODE_SHADED);
			ImGui::SameLine(); ImGui::RadioButton("Solid color", (int*)&m_UIState.WireframeMode, (int)UIState::WireframeMode::WIREFRAME_MODE_SOLID_COLOR);
			if (m_UIState.WireframeMode == UIState::WireframeMode::WIREFRAME_MODE_SOLID_COLOR)
				ImGui::ColorEdit3("Wire solid color", m_UIState.WireframeColor, ImGuiColorEditFlags_NoAlpha);
		}

		ImGui::Spacing();
		ImGui::Spacing();

		if (ImGui::CollapsingHeader("Presentation Mode", ImGuiTreeNodeFlags_DefaultOpen))
		{
			const char* fullscreenModes[] = { "Windowed", "BorderlessFullscreen", "ExclusiveFulscreen" };
			if (ImGui::Combo("Fullscreen Mode", (int*)&m_fullscreenMode, fullscreenModes, _countof(fullscreenModes)))
			{
				if (m_previousFullscreenMode != m_fullscreenMode)
				{
					HandleFullScreen();
					m_previousFullscreenMode = m_fullscreenMode;
				}
			}
		}

		ImGui::Spacing();
		ImGui::Spacing();

		if (m_FreesyncHDROptionEnabled && ImGui::CollapsingHeader("FreeSync HDR", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static bool openWarning = false;
			const char** displayModeNames = &m_displayModesNamesAvailable[0];
			if (ImGui::Combo("Display Mode", (int*)&m_currentDisplayModeNamesIndex, displayModeNames, (int)m_displayModesNamesAvailable.size()))
			{
				if (m_fullscreenMode != PRESENTATIONMODE_WINDOWED)
				{
					UpdateDisplay();
					m_previousDisplayModeNamesIndex = m_currentDisplayModeNamesIndex;
				}
				else if (CheckIfWindowModeHdrOn() &&
					(m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DISPLAYMODE_SDR ||
						m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DISPLAYMODE_HDR10_2084 ||
						m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DISPLAYMODE_HDR10_SCRGB))
				{
					UpdateDisplay();
					m_previousDisplayModeNamesIndex = m_currentDisplayModeNamesIndex;
				}
				else
				{
					openWarning = true;
					m_currentDisplayModeNamesIndex = m_previousDisplayModeNamesIndex;
				}
			}

			if (openWarning)
			{
				ImGui::OpenPopup("Display Modes Warning");
				ImGui::BeginPopupModal("Display Modes Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize);
				ImGui::Text("\nChanging display modes is only available either using HDR toggle in windows display setting for HDR10 modes or in fullscreen for FS HDR modes\n\n");
				if (ImGui::Button("Cancel", ImVec2(120, 0))) { openWarning = false; ImGui::CloseCurrentPopup(); }
				ImGui::EndPopup();
			}

			if (m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DisplayMode::DISPLAYMODE_FSHDR_Gamma22 ||
				m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DisplayMode::DISPLAYMODE_FSHDR_SCRGB)
			{
				if (ImGui::Checkbox("Enable Local Dimming", &m_enableLocalDimming))
				{
					OnLocalDimmingChanged();
				}
			}
		}

		ImGui::End(); // CONTROLS
	}


	// Render PROFILER window
	//
	if (m_UIState.bShowProfilerWindow)
	{
		constexpr size_t NUM_FRAMES = 128;
		static float FRAME_TIME_ARRAY[NUM_FRAMES] = { 0 };

		// track highest frame rate and determine the max value of the graph based on the measured highest value
		static float RECENT_HIGHEST_FRAME_TIME = 0.0f;
		constexpr int FRAME_TIME_GRAPH_MAX_FPS[] = { 800, 240, 120, 90, 60, 45, 30, 15, 10, 5, 4, 3, 2, 1 };
		static float  FRAME_TIME_GRAPH_MAX_VALUES[_countof(FRAME_TIME_GRAPH_MAX_FPS)] = { 0 }; // us
		for (int i = 0; i < _countof(FRAME_TIME_GRAPH_MAX_FPS); ++i) { FRAME_TIME_GRAPH_MAX_VALUES[i] = 1000000.f / FRAME_TIME_GRAPH_MAX_FPS[i]; }

		//scrolling data and average FPS computing
		const std::vector<TimeStamp>& timeStamps = m_pRenderer->GetTimingValues();
		const bool bTimeStampsAvailable = timeStamps.size() > 0;
		if (bTimeStampsAvailable)
		{
			RECENT_HIGHEST_FRAME_TIME = 0;
			FRAME_TIME_ARRAY[NUM_FRAMES - 1] = timeStamps.back().m_microseconds;
			for (uint32_t i = 0; i < NUM_FRAMES - 1; i++)
			{
				FRAME_TIME_ARRAY[i] = FRAME_TIME_ARRAY[i + 1];
			}
			RECENT_HIGHEST_FRAME_TIME = std::max(RECENT_HIGHEST_FRAME_TIME, FRAME_TIME_ARRAY[NUM_FRAMES - 1]);
		}
		const float& frameTime_us = FRAME_TIME_ARRAY[NUM_FRAMES - 1];
		const float  frameTime_ms = frameTime_us * 0.001f;
		const int fps = bTimeStampsAvailable ? static_cast<int>(1000000.0f / frameTime_us) : 0;

		// UI
		ImGui::SetNextWindowPos(ImVec2((float)PROFILER_WINDOW_POS_X, (float)PROFILER_WINDOW_POS_Y), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(PROFILER_WINDOW_SIZE_X, PROFILER_WINDOW_SIZE_Y), ImGuiCond_FirstUseEver);
		ImGui::Begin("PROFILER (F2)", &m_UIState.bShowProfilerWindow);

		ImGui::Text("Resolution : %ix%i", m_Width, m_Height);
		ImGui::Text("API        : %s", m_systemInfo.mGfxAPI.c_str());
		ImGui::Text("GPU        : %s", m_systemInfo.mGPUName.c_str());
		ImGui::Text("CPU        : %s", m_systemInfo.mCPUName.c_str());
		ImGui::Text("FPS        : %d (%.2f ms)", fps, frameTime_ms);

		if (ImGui::CollapsingHeader("GPU Timings", ImGuiTreeNodeFlags_DefaultOpen))
		{
			std::string msOrUsButtonText = m_UIState.bShowMilliseconds ? "Switch to microseconds(us)" : "Switch to milliseconds(ms)";
			if (ImGui::Button(msOrUsButtonText.c_str())) {
				m_UIState.bShowMilliseconds = !m_UIState.bShowMilliseconds;
			}
			ImGui::Spacing();

			if (m_isCpuValidationLayerEnabled || m_isGpuValidationLayerEnabled)
			{
				ImGui::TextColored(ImVec4(1, 1, 0, 1), "WARNING: Validation layer is switched on");
				ImGui::Text("Performance numbers may be inaccurate!");
			}

			// find the index of the FrameTimeGraphMaxValue as the next higher-than-recent-highest-frame-time in the pre-determined value list
			size_t iFrameTimeGraphMaxValue = 0;
			for (uint64_t i = 0; i < _countof(FRAME_TIME_GRAPH_MAX_VALUES); ++i)
			{
				if (RECENT_HIGHEST_FRAME_TIME < FRAME_TIME_GRAPH_MAX_VALUES[i]) // FRAME_TIME_GRAPH_MAX_VALUES are in increasing order
				{
					iFrameTimeGraphMaxValue = std::min(_countof(FRAME_TIME_GRAPH_MAX_VALUES) - 1, i + 1);
					break;
				}
			}
			ImGui::PlotLines("", FRAME_TIME_ARRAY, NUM_FRAMES, 0, "GPU frame time (us)", 0.0f, FRAME_TIME_GRAPH_MAX_VALUES[iFrameTimeGraphMaxValue], ImVec2(0, 80));

			for (uint32_t i = 0; i < timeStamps.size(); i++)
			{
				float value = m_UIState.bShowMilliseconds ? timeStamps[i].m_microseconds / 1000.0f : timeStamps[i].m_microseconds;
				const char* pStrUnit = m_UIState.bShowMilliseconds ? "ms" : "us";
				ImGui::Text("%-18s: %7.2f %s", timeStamps[i].m_label.c_str(), value, pStrUnit);
			}
		}
		ImGui::End(); // PROFILER
	}
}

void UIState::Initialize()
{
	// init magnifier params
	for (int ch = 0; ch < 3; ++ch) this->MagnifierParams.fBorderColorRGB[ch] = MAGNIFIER_BORDER_COLOR__FREE[ch]; // start at 'free' state

	// init GUI state
	this->SelectedTonemapperIndex = 0;
	this->bUseTAA = true;
	this->bUseMagnifier = false;
	this->bLockMagnifierPosition = this->bLockMagnifierPositionHistory = false;
	this->SelectedSkydomeTypeIndex = 0;
	this->Exposure = 1.0f;
	this->IBLFactor = 2.0f;
	this->EmissiveFactor = 1.0f;
	this->bDrawLightFrustum = false;
	this->bDrawBoundingBoxes = false;
	this->WireframeMode = WireframeMode::WIREFRAME_MODE_OFF;
	this->WireframeColor[0] = 0.0f;
	this->WireframeColor[1] = 1.0f;
	this->WireframeColor[2] = 0.0f;
	this->bShowControlsWindow = true;
	this->bShowProfilerWindow = true;
}



//
// Magnifier UI Controls
//
void UIState::ToggleMagnifierLock()
{
	if (this->bUseMagnifier)
	{
		this->bLockMagnifierPositionHistory = this->bLockMagnifierPosition; // record histroy
		this->bLockMagnifierPosition = !this->bLockMagnifierPosition; // flip state
		const bool bLockSwitchedOn = !this->bLockMagnifierPositionHistory && this->bLockMagnifierPosition;
		const bool bLockSwitchedOff = this->bLockMagnifierPositionHistory && !this->bLockMagnifierPosition;
		if (bLockSwitchedOn)
		{
			const ImGuiIO& io = ImGui::GetIO();
			this->LockedMagnifiedScreenPositionX = static_cast<int>(io.MousePos.x);
			this->LockedMagnifiedScreenPositionY = static_cast<int>(io.MousePos.y);
			for (int ch = 0; ch < 3; ++ch) this->MagnifierParams.fBorderColorRGB[ch] = MAGNIFIER_BORDER_COLOR__LOCKED[ch];
		}
		else if (bLockSwitchedOff)
		{
			for (int ch = 0; ch < 3; ++ch) this->MagnifierParams.fBorderColorRGB[ch] = MAGNIFIER_BORDER_COLOR__FREE[ch];
		}
	}
}
// These are currently not bound to any mouse input and are here for convenience/reference.
// Mouse scroll is currently wired up to camera for panning and moving in the local Z direction.
// Any application that would prefer otherwise can utilize these for easily controlling the magnifier parameters through the desired input.
void UIState::AdjustMagnifierSize(float increment /*= 0.05f*/) { MagnifierParams.fMagnifierScreenRadius = clamped(MagnifierParams.fMagnifierScreenRadius + increment, MAGNIFIER_RADIUS_MIN, MAGNIFIER_RADIUS_MAX); }
void UIState::AdjustMagnifierMagnification(float increment /*= 1.00f*/) { MagnifierParams.fMagnificationAmount = clamped(MagnifierParams.fMagnificationAmount + increment, MAGNIFICATION_AMOUNT_MIN, MAGNIFICATION_AMOUNT_MAX); }


void ComputeFrustumPlanes(glm::vec4* frustum, const float* clip)
{
	frustum[0].x = clip[3] - clip[0];
	frustum[0].y = clip[7] - clip[4];
	frustum[0].z = clip[11] - clip[8];
	frustum[0].w = clip[15] - clip[12];

	frustum[1].x = clip[3] + clip[0];
	frustum[1].y = clip[7] + clip[4];
	frustum[1].z = clip[11] + clip[8];
	frustum[1].w = clip[15] + clip[12];

	frustum[2].x = clip[3] + clip[1];
	frustum[2].y = clip[7] + clip[5];
	frustum[2].z = clip[11] + clip[9];
	frustum[2].w = clip[15] + clip[13];

	frustum[3].x = clip[3] - clip[1];
	frustum[3].y = clip[7] - clip[5];
	frustum[3].z = clip[11] - clip[9];
	frustum[3].w = clip[15] - clip[13];

	frustum[4].x = clip[3] - clip[2];
	frustum[4].y = clip[7] - clip[6];
	frustum[4].z = clip[11] - clip[10];
	frustum[4].w = clip[15] - clip[14];

	frustum[5].x = clip[3] + clip[2];
	frustum[5].y = clip[7] + clip[6];
	frustum[5].z = clip[11] + clip[10];
	frustum[5].w = clip[15] + clip[14];

	for (int i = 0; i < 6; i++)
	{
		//frustum[i].normalize();
		frustum[i] = glm::normalize(frustum[i]);
	}
}
glm::vec4 makeVect(float _x, float _y, float _z = 0.f, float _w = 0.f)
{
	glm::vec4 res; res.x = _x; res.y = _y; res.z = _z; res.w = _w;
	return res;
}
static float DistanceToPlane(const glm::vec4& point, const glm::vec4& plan)
{
	auto p3 = glm::vec3(point);
	auto pl3 = glm::vec3(plan);
	auto r = glm::dot(p3, pl3);
	return r + plan.w;
	//return plan.dot3(point) + plan.w;
}

glm::vec4 Lerp(glm::vec4 r, const glm::vec4& v, float t)
{
	r.x += (v.x - r.x) * t;
	r.y += (v.y - r.y) * t;
	r.z += (v.z - r.z) * t;
	r.w += (v.w - r.w) * t;
	return r;
}

glm::vec4 TransformPoint(glm::vec4 ps, const glm::mat4& matrix)
{
	glm::vec4 out;

	out.x = ps.x * matrix[0][0] + ps.y * matrix[1][0] + ps.z * matrix[2][0] + matrix[3][0];
	out.y = ps.x * matrix[0][1] + ps.y * matrix[1][1] + ps.z * matrix[2][1] + matrix[3][1];
	out.z = ps.x * matrix[0][2] + ps.y * matrix[1][2] + ps.z * matrix[2][2] + matrix[3][2];
	out.w = ps.x * matrix[0][3] + ps.y * matrix[1][3] + ps.z * matrix[2][3] + matrix[3][3];

	return out;
}
static glm::vec2 worldToPos(const glm::vec4& worldPos, const glm::mat4& mat
	, glm::vec2 position /*= ImVec2(gContext.mX, gContext.mY)*/, glm::vec2 size /*= ImVec2(gContext.mWidth, gContext.mHeight)*/)
{
	glm::vec4 trans = TransformPoint(worldPos, mat);
	//trans.TransformPoint(worldPos, mat);
	trans *= 0.5f / trans.w;
	trans += makeVect(0.5f, 0.5f);
	trans.y = 1.f - trans.y;
	trans.x *= size.x;
	trans.y *= size.y;
	trans.x += position.x;
	trans.y += position.y;
	return glm::vec2(trans.x, trans.y);
}
struct v4col
{
	glm::vec4 v;
	uint32_t col;
	float thickness;
};
void DrawGrid(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& matrix, const float gridSize, glm::vec4 rc)
{
	glm::vec2 ps = { rc.x,rc.y }, size = { rc.z,rc.w };
	auto dw = ImGui::GetWindowDrawList();
	auto viewProjection = projection * view;
	glm::vec4 frustum[6];
	ComputeFrustumPlanes(frustum, (float*)&viewProjection);
	auto res = matrix * viewProjection;
	std::vector<v4col> pv;
	for (float f = -gridSize; f <= gridSize; f += 1.f)
	{
		for (int dir = 0; dir < 2; dir++)
		{
			glm::vec4 ptA = makeVect(dir ? -gridSize : f, 0.f, dir ? f : -gridSize);
			glm::vec4 ptB = makeVect(dir ? gridSize : f, 0.f, dir ? f : gridSize);
			bool visible = true;
			for (int i = 0; i < 6; i++)
			{
				float dA = DistanceToPlane(ptA, frustum[i]);
				float dB = DistanceToPlane(ptB, frustum[i]);
				if (dA < 0.f && dB < 0.f)
				{
					visible = false;
					break;
				}
				if (dA > 0.f && dB > 0.f)
				{
					continue;
				}
				if (dA < 0.f)
				{
					float len = fabsf(dA - dB);
					float t = fabsf(dA) / len;
					//ptA.Lerp(ptB, t);
					ptA = Lerp(ptA, ptB, t);
				}
				if (dB < 0.f)
				{
					float len = fabsf(dB - dA);
					float t = fabsf(dB) / len;
					//ptB.Lerp(ptA, t);
					ptB = Lerp(ptB, ptA, t);
				}
			}
			if (visible)
			{
				ImU32 col = IM_COL32(0x80, 0x80, 0x80, 0xFF);
				col = (fmodf(fabsf(f), 10.f) < FLT_EPSILON) ? IM_COL32(0x90, 0x90, 0x90, 0xFF) : col;
				col = (fabsf(f) < FLT_EPSILON) ? IM_COL32(0x40, 0x40, 0x40, 0xFF) : col;

				float thickness = 1.f;
				if ((fmodf(fabsf(f), 10.f) < FLT_EPSILON))
				{
					thickness = 1.5f;
					// x y z 中线
					unsigned int c3[] = { IM_COL32(0x8a, 0xdb, 0x00, 0x80), IM_COL32(0xff, 0x36, 0x53, 0x80) ,IM_COL32(0x2d, 0x8f, 0xff, 0x80), };

					if (f == 0.0f)
					{
						col = c3[dir];
					}
				}
				thickness = (fabsf(f) < FLT_EPSILON) ? 2.3f : thickness;

				pv.push_back({ glm::vec4{ worldToPos(ptA, res, ps, size), worldToPos(ptB, res, ps, size) }, col, thickness });
			}
		}
	}
	for (auto& i : pv)
	{
		auto& ps = i.v;
		glm::vec2 pss[2] = { {ps.x, ps.y},{ps.z, ps.w} };
		glm::vec2 a = { 0.5,0.5 };
		auto np = glm::mix(pss[0], pss[1], a);
		//dw->AddLine(ImVec2(ps.x, ps.y), ImVec2(np.x, np.y), i.col, i.thickness);
		//dw->AddLine(ImVec2(np.x, np.y), ImVec2(ps.z, ps.w), i.col, i.thickness);
		dw->AddLine(ImVec2(ps.x, ps.y), ImVec2(ps.z, ps.w), i.col, i.thickness);
	}
}
