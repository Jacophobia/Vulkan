#pragma once

#include <glm/glm.hpp>

class Actor
{
    // state
    glm::vec3 position_;
    glm::vec3 scale_;
    glm::vec3 world_up_;
    
    // Euler angles (in degrees)
    float yaw_;
    float pitch_;
    float roll_;

    glm::vec3 get_front() const;
    glm::vec3 get_up() const;
    glm::vec3 get_right() const;
public:
    Actor(glm::vec3 position, glm::vec3 scale, glm::vec3 world_up, float yaw, float pitch, float roll);
    void yaw(float amount);
    void pitch(float amount);
    void roll(float amount);

    void move(glm::vec3 offset);

    glm::mat4 get_transform() const;
};
