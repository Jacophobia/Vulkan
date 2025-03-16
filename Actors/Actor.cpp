#include "Actor.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

glm::vec3 Actor::get_front() const
{
    glm::vec3 f;
    f.x = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    f.y = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    f.z = sin(glm::radians(pitch_));
    return normalize(f);
}

glm::vec3 Actor::get_up() const
{
    return normalize(cross(get_right(), get_front()));
}

glm::vec3 Actor::get_right() const
{
    return normalize(cross(get_front(), world_up_));
}

Actor::Actor(const glm::vec3 position, const glm::vec3 scale, const glm::vec3 world_up, const float yaw, const float pitch, const float roll)
    : position_(position), scale_(scale), world_up_(world_up), yaw_(yaw), pitch_(pitch), roll_(roll)
{
    
}

void Actor::yaw(const float amount)
{
    yaw_ += amount;
}

void Actor::pitch(const float amount)
{
    pitch_ += amount;
}

void Actor::roll(const float amount)
{
    roll_ += amount;
}

void Actor::move(const glm::vec3 offset)
{
    position_ += offset;
}

glm::mat4 Actor::get_transform() const
{
    glm::mat4 transform(1.f);
        
    transform = translate(transform, position_);
    transform *= glm::yawPitchRoll(glm::radians(yaw_), glm::radians(roll_), glm::radians(pitch_));
    transform = scale(transform, scale_);

    return transform;
}
