#include "Camera.h"

// #include <glm/ext/matrix_clip_space.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <algorithm> // for std::clamp
#include <format>

#include "../Logging/Logging.h"

Camera::Camera(glm::vec3 start_pos, glm::vec3 up_vector, float start_yaw, float start_pitch,
               float start_fov, float aspect_ratio, float near_p, float far_p)
    : position_(start_pos), world_up_(up_vector), yaw_(start_yaw), pitch_(start_pitch),
      fov_(start_fov), aspect_(aspect_ratio), near_plane_(near_p), far_plane_(far_p),
      movement_speed_(2.5f), mouse_sensitivity_(0.1f)
{
    update_camera_vectors();
}

// Returns a calculated UniformBufferObject based on the current camera state.
UniformBufferObject Camera::get_ubo() const {
    UniformBufferObject ubo;

    // The view matrix is calculated using the camera's position and front vector.
    if (!target_.has_value())
    {
        ubo.view = glm::lookAt(position_, position_ + front_, up_);
    }
    else
    {
        ubo.view = glm::lookAt(position_, target_.value()(), up_);
    }
    // The projection matrix uses a perspective projection.
    ubo.proj = glm::perspective(glm::radians(fov_), aspect_, near_plane_, far_plane_);
    // Vulkan's clip space inverts the Y coordinate.
    ubo.proj[1][1] *= -1;
    return ubo;
}

// Camera manipulation methods:
void Camera::set_position(const glm::vec3& new_position) {
    position_ = new_position;
}

void Camera::move(const glm::vec3& offset) {
    position_ += offset;
}

// Set absolute rotation angles.
void Camera::set_rotation(const float new_yaw, const float new_pitch) {
    yaw_ = new_yaw;
    pitch_ = new_pitch;
    constrain_pitch();
    update_camera_vectors();
}

// Rotate relative to current orientation.
void Camera::rotate(const float delta_yaw, const float delta_pitch) {
    yaw_ += delta_yaw;
    pitch_ += delta_pitch;
    constrain_pitch();
    update_camera_vectors();
}

// Set a new field-of-view.
void Camera::set_fov(const float new_fov) {
    fov_ = new_fov;
}

// Set a new aspect ratio.
void Camera::set_aspect_ratio(const float new_aspect) {
    aspect_ = new_aspect;
}

// Set new near and far clipping planes.
void Camera::set_clipping_planes(const float new_near, const float new_far) {
    near_plane_ = new_near;
    far_plane_ = new_far;
}

// Process keyboard input to move the camera.
// "FORWARD", "BACKWARD", "LEFT", and "RIGHT" are supported.
void Camera::process_keyboard(const std::string& direction, const float delta_time) {
    const float velocity = movement_speed_ * delta_time;
    if (direction == "FORWARD")
        position_ += front_ * velocity;
    else if (direction == "BACKWARD")
        position_ -= front_ * velocity;
    else if (direction == "LEFT")
        position_ -= right_ * velocity;
    else if (direction == "RIGHT")
        position_ += right_ * velocity;
}

// Process mouse movement input to adjust the camera's yaw and pitch.
void Camera::process_mouse_movement(float x_offset, float y_offset, const bool should_constrain_pitch) {
    x_offset *= mouse_sensitivity_;
    y_offset *= mouse_sensitivity_;
    
    yaw_   += x_offset;
    pitch_ += y_offset;
    
    if (should_constrain_pitch) {
        constrain_pitch();
    }
    
    update_camera_vectors();
}

// Process mouse scroll input to change the FOV.
void Camera::process_mouse_scroll(const float y_offset) {
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
void Camera::update_camera_vectors() {
    glm::vec3 f;
    f.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    f.y = sin(glm::radians(pitch_));
    f.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front_ = glm::normalize(f);
    
    // Recalculate right and up vectors.
    right_ = glm::normalize(glm::cross(front_, world_up_));
    up_    = {0,0,1};// glm::normalize(glm::cross(right_, front_)); // TODO: make the up logic more intelligent
}

// Clamp the pitch to avoid gimbal lock (flipping).
void Camera::constrain_pitch() {
    pitch_ = std::clamp(pitch_, -89.0f, 89.0f);
}


