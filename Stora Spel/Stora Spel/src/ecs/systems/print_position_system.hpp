#ifndef PRINT_POSITION_SYSTEM_HPP_
#define PRINT_POSITION_SYSTEM_HPP_

#include "position.hpp"
#include "physics_component.hpp"

void Print(entt::registry &registry) {
  registry.view<Position, PhysicsComponent>().each([](auto &pos, auto &vel) {
    std::cout << "Pos X: " << pos.x << " Pos Y: " << pos.y << " Pos Z: " << pos.z << "\n";
  });
}

#endif  // PRINT_POSITION_SYSTEM_HPP_
