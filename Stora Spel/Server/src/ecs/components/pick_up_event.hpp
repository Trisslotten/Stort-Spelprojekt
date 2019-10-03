#ifndef PICK_UP_EVENT_HPP_
#define PICK_UP_EVENT_HPP_

#include <shared.hpp>

struct PickUpEvent {
  int pick_up_id;
  PlayerID player_id;
  AbilityID ability_id;
};

#endif  // PICK_UP_EVENT_HPP_