#pragma once

using Control = const char*;

namespace controls
{

constexpr Control MOVE_RIGHT = "move_right"; // controller: left joystick x-axis or keyboard: a & d (a is negative)
constexpr Control MOVE_UP = "move_up"; // controller: left joystick y-axis or keyboard: w & s (s is negative)

constexpr Control LOOK_RIGHT = "look_right"; // controller: right joystick x-axis or keyboard: mouse x-axis
constexpr Control LOOK_UP = "look_up"; // controller: right joystick y-axis or keyboard: mouse y-axis

constexpr Control ABILITY_ONE = "ability_one"; // controller: A or keyboard: 1
constexpr Control ABILITY_TWO = "ability_two"; // controller: B or keyboard: 2
constexpr Control ABILITY_THREE = "ability_three"; // controller: X or keyboard: 3
constexpr Control ABILITY_FOUR = "ability_four"; // controller: Y or keyboard: 4
constexpr Control ABILITY_FIVE = "ability_five"; // controller: Left Bumper or keyboard: 5
constexpr Control ABILITY_SIX = "ability_six"; // controller: Left Trigger or keyboard: 6
constexpr Control ABILITY_SEVEN = "ability_seven"; // controller: Right Bumper or keyboard: 7
constexpr Control ABILITY_EIGHT = "ability_eight"; // controller: Right Trigger or keyboard: 8
constexpr Control ABILITY_NINE = "ability_nine"; // controller: Left Joystick Button or keyboard: 9
constexpr Control ABILITY_TEN = "ability_ten"; // controller: Right Joystick Button or keyboard: 0

constexpr Control PAUSE = "pause"; // controller: Start or keyboard: Escape
constexpr Control SELECT = "select"; // controller: Select or keyboard: M

}