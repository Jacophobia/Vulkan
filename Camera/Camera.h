#pragma once

#include <functional>

#include "../Rendering/UniformBufferObject.h"

#include <optional>

class Camera {
    // Camera state
    glm::vec3 position_;
    glm::vec3 front_;
    glm::vec3 up_;
    glm::vec3 right_;
    glm::vec3 world_up_;
    
    // Euler angles (in degrees)
    float yaw_;
    float pitch_;
    
    // Projection parameters
    float fov_;        // Field of view in degrees
    float aspect_;     // Aspect ratio (width / height)
    float near_plane_;  
    float far_plane_;
    
    // Movement options
    float movement_speed_;
    float sensitivity_;

    // overrides
    std::optional<std::function<glm::vec3()>> target_;
    
public:
    // Constructor with sensible defaults.
    Camera(glm::vec3 start_pos = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 up_vector = glm::vec3(0.0f, 0.0f, 1.0f),
           float start_yaw = 0.f, float start_pitch = 0.f,
           float start_fov = 45.0f, float aspect_ratio = 4.0f / 3.0f,
           float near_p = 0.1f, float far_p = 100.0f);
    
    // Returns a calculated UniformBufferObject based on the current camera state.
    [[nodiscard]] UniformBufferObject get_ubo() const;
    
    // Camera manipulation methods:
    void set_position(const glm::vec3& new_position);
    
    void move(const glm::vec3& offset);
    
    // Set absolute rotation angles.
    void set_rotation(float new_yaw, float new_pitch);
    
    // Rotate relative to current orientation.
    void rotate(float delta_yaw, float delta_pitch);
    
    // Set a new field-of-view.
    void set_fov(float new_fov);
    
    // Set a new aspect ratio.
    void set_aspect_ratio(float new_aspect);
    
    // Set new near and far clipping planes.
    void set_clipping_planes(float new_near, float new_far);
    
    // Process keyboard input to move the camera.
    // "FORWARD", "BACKWARD", "LEFT", and "RIGHT" are supported.
    void travel(float x_offset, float y_offset, float delta_time);
    
    // Process mouse movement input to adjust the camera's yaw and pitch.
    void look(float x_offset, float y_offset, float delta_time, bool should_constrain_pitch = true);
    
    // Process mouse scroll input to change the FOV.
    void process_mouse_scroll(float y_offset);

    // set a target location to focus on
    void set_target(std::function<glm::vec3()>& target);

    // forget the target
    void reset_target();
    
private:
    // Recalculate the camera's direction vectors from the current Euler angles.
    void update_camera_vectors();
    
    // Clamp the pitch to avoid gimbal lock (flipping).
    void constrain_pitch();
};

