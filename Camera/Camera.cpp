﻿#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm> // for std::clamp

Camera::Camera(glm::vec3 start_pos, glm::vec3 up_vector, float start_yaw, float start_pitch,
               float start_fov, float aspect_ratio, float near_p, float far_p)
    : position_(start_pos), world_up_(up_vector), yaw_(start_yaw), pitch_(start_pitch),
      fov_(start_fov), aspect_(aspect_ratio), near_plane_(near_p), far_plane_(far_p),
      movement_speed_(0.25f), sensitivity_(75.f)
{
    update_camera_vectors();
}

// Returns a calculated UniformBufferObject based on the current camera state.
UniformBufferObject Camera::get_ubo() const
{
    UniformBufferObject ubo;

    // The view matrix is calculated using the camera's position and front vector.
    ubo.view = lookAt(position_, position_ + front_, up_);

    // The projection matrix uses a perspective projection.
    ubo.proj = glm::infinitePerspective(glm::radians(fov_), aspect_, near_plane_);

    // Vulkan's clip space inverts the Y coordinate.
    ubo.proj[1][1] *= -1;
    return ubo;
}

// Camera manipulation methods:
void Camera::set_position(const glm::vec3& new_position)
{
    position_ = new_position;
}

void Camera::move(const glm::vec3& offset) 
{
    position_ += offset;
}

// Set absolute rotation angles.
void Camera::set_rotation(const float new_yaw, const float new_pitch) 
{
    yaw_ = new_yaw;
    pitch_ = new_pitch;
    constrain_pitch();
    update_camera_vectors();
}

// Rotate relative to current orientation.
void Camera::rotate(const float delta_yaw, const float delta_pitch) 
{
    yaw_ += delta_yaw;
    pitch_ += delta_pitch;
    constrain_pitch();
    update_camera_vectors();
}

// Set a new field-of-view.
void Camera::set_fov(const float new_fov) 
{
    fov_ = new_fov;
}

// Set a new aspect ratio.
void Camera::set_aspect_ratio(const float new_aspect) 
{
    aspect_ = new_aspect;
}

// Set new near and far clipping planes.
void Camera::set_clipping_planes(const float new_near, const float new_far) 
{
    near_plane_ = new_near;
    far_plane_ = new_far;
}

// Process keyboard input to move the camera.
void Camera::travel(float x_offset, float y_offset, float z_offset, const float delta_time) 
{
    const float velocity = movement_speed_ * delta_time;
    position_ += front_ * velocity * y_offset;
    position_ += right_ * velocity * x_offset;
    position_ += up_ * velocity * z_offset;
}

// Process mouse movement input to adjust the camera's yaw and pitch.
void Camera::look(float x_offset, float y_offset, const float delta_time, const bool should_constrain_pitch) 
{
    x_offset *= sensitivity_ * delta_time;
    y_offset *= sensitivity_ * delta_time;
    
    yaw_   += x_offset;
    pitch_ += y_offset;
    
    if (should_constrain_pitch) 
    {
        constrain_pitch();
    }
    
    update_camera_vectors();
}

// Process mouse scroll input to change the FOV.
void Camera::process_mouse_scroll(const float y_offset) 
{
    fov_ -= y_offset;
    fov_ = std::clamp(fov_, 1.0f, 45.0f);
}

void Camera::set_target(std::function<glm::vec3()>& target)
{
    target_ = target;
}

void Camera::reset_target()
{
    target_.reset();
}

// Recalculate the camera's direction vectors from the current Euler angles.
void Camera::update_camera_vectors() 
{
    if (target_.has_value())
    {
        front_ = normalize(target_.value()() - position_);
    }
    else
    {
        glm::vec3 f;
        f.x = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
        f.y = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
        f.z = sin(glm::radians(pitch_));
        front_ = normalize(f);
    }
    
    // Recalculate right and up vectors.
    right_ = normalize(cross(front_, world_up_));
    up_    = normalize(cross(right_, front_));
}

// Clamp the pitch to avoid gimbal lock (flipping).
void Camera::constrain_pitch() 
{
    pitch_ = std::clamp(pitch_, -89.0f, 89.0f);
}


