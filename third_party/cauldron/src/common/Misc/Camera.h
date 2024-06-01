// AMD Cauldron code
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

#pragma once

#include "../../libs/vectormath/vectormath.hpp"

#include "../glm/gtc/matrix_transform.hpp"

class Camera
{
public:
    Camera();
    void SetMatrix(const glm::mat4& cameraMatrix);
    void LookAt(const glm::vec4& eyePos, const glm::vec4& lookAt);
    void LookAt(float yaw, float pitch, float distance, const glm::vec4& at);
    void SetFov(float fov, uint32_t width, uint32_t height, float nearPlane, float farPlane);
    void SetFov(float fov, float aspectRatio, float nearPlane, float farPlane);
    void UpdateCameraPolar(float yaw, float pitch, float x, float y, float distance);
    void UpdateCameraWASD(float yaw, float pitch, const bool keyDown[256], double deltaTime);

    glm::mat4 GetView() const { return m_View; }
    glm::mat4 GetPrevView() const { return m_PrevView; }
    glm::vec4 GetPosition() const { return m_eyePos;   }

	
    glm::vec4 GetDirection()    const { return glm::vec4(glm::vec3(glm::transpose(m_View) * glm::vec4(0.0f, 0.0f, 1.0f,0.0f)), 0); }
    glm::vec4 GetUp()           const { return glm::vec4(glm::vec3(glm::transpose(m_View) * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)), 0); }
    glm::vec4 GetSide()         const { return glm::vec4(glm::vec3(glm::transpose(m_View) * glm::vec4(1.0f, 1.0f, 0.0f, 0.0f)), 0); }
    glm::mat4 GetProjection()   const { return m_Proj; }

    float GetFovH() const { return m_fovH; }
    float GetFovV() const { return m_fovV; }

    float GetAspectRatio() const { return m_aspectRatio; }

    float GetNearPlane() const { return m_near; }
    float GetFarPlane() const { return m_far; }

    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }
    float GetDistance() const { return m_distance; }

    void SetSpeed( float speed ) { m_speed = speed; }
    void SetProjectionJitter(float jitterX, float jitterY);
    void SetProjectionJitter(uint32_t width, uint32_t height, uint32_t &seed);
    void UpdatePreviousMatrices() { m_PrevView = m_View; }

private:
    glm::mat4       m_View;
    glm::mat4       m_Proj;
    glm::mat4       m_PrevView;
    glm::vec4       m_eyePos;
    float               m_distance;
    float               m_fovV, m_fovH;
    float               m_near, m_far;
    float               m_aspectRatio;

    float               m_speed = 1.0f;
    float               m_yaw = 0.0f;
    float               m_pitch = 0.0f;
    float               m_roll = 0.0f;
    bool flipY = false;
};

glm::vec4 PolarToVector(float roll, float pitch);
glm::mat4 LookAtRH(const glm::vec4& eyePos, const glm::vec4& lookAt, bool flipY);
glm::vec4 MoveWASD(const bool keyDown[256]);

glm::vec4 get_row(const glm::mat4& m, int n);