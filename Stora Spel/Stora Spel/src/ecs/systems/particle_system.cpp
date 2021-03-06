#include "particle_system.hpp"

#include "engine.hpp"

void ParticleSystem::Init(Engine* engine) { this->engine_ = engine; }

void ParticleSystem::Update(entt::registry& registry, float dt) {
  auto view_movable = registry.view<ParticleComponent, TransformComponent>();
  for (auto& entity : view_movable) {
    auto& particle_c = view_movable.get<ParticleComponent>(entity);
    auto& transform_c = view_movable.get<TransformComponent>(entity);

    for (int i = 0; i < particle_c.handles.size(); ++i) {
      glob::SetEmitPosition(particle_c.handles[i],
                            transform_c.position + particle_c.offsets[i]);
      glob::SetParticleDirection(
          particle_c.handles[i],
          transform_c.rotation * particle_c.directions[i]);
    }
  }

  auto view_model_movable =
      registry.view<ParticleComponent, TransformComponent, FollowBoneComponent,
                    AnimationComponent, ModelComponent, PlayerComponent>();
  for (auto& entity : view_model_movable) {
    auto& particle_c = view_model_movable.get<ParticleComponent>(entity);
    auto& transform_c = view_model_movable.get<TransformComponent>(entity);
    auto& follow_c = view_model_movable.get<FollowBoneComponent>(entity);
    auto& animation_c = view_model_movable.get<AnimationComponent>(entity);
    auto& model_c = view_model_movable.get<ModelComponent>(entity);
    auto& player_c = view_model_movable.get<PlayerComponent>(entity);

    PhysicsComponent* physics_c = nullptr;
    if (registry.has<PhysicsComponent>(entity)) {
      physics_c = &registry.get<PhysicsComponent>(entity);
    }

    if (animation_c.bone_transforms.empty()) {
      continue;
    }
    for (int i = 0; i < follow_c.emitters.size(); ++i) {
      BoneEmitter& current = follow_c.emitters[i];
      int id = current.bone_id;
      auto anim_trans = animation_c.bone_transforms[id];

      const glm::mat4 pre_rotation =
          glm::rotate(glm::pi<float>() / 2.f, glm::vec3(0, 1, 0)) *
          glm::rotate(-glm::pi<float>() / 2.f, glm::vec3(1, 0, 0));
      glm::mat4 model_trans =
          glm::translate(transform_c.position) *
          glm::toMat4(transform_c.rotation + model_c.rot_offset) *
          glm::translate(-model_c.offset) * glm::scale(transform_c.scale);

      model_trans = model_trans * pre_rotation;

      glm::mat4 trans = model_trans * anim_trans;

      glm::mat3 normal_transform =
          glm::transpose(glm::inverse(glm::mat3(trans)));
      glm::vec3 local_direction = normalize(current.dir);
      glm::vec3 direction = normalize(normal_transform * local_direction);

      glm::vec3 pos = glm::vec3(trans * glm::vec4(current.pos, 1));

      glm::vec3 vel = current.speed * direction;

      glm::vec3 emitter_vel = glm::vec3(0);
      if (physics_c) {
        emitter_vel += physics_c->velocity;
      }
      std::unordered_map<std::string, std::string> map;
      switch (current.type) {
        case BoneEmitterType::ROCKET:
          if (player_c.sprinting && !model_c.invisible) {
            map["spawn_rate"] = "500.f";
          } else {
            map["spawn_rate"] = "0.f";
          }
          break;
        case BoneEmitterType::SLIDE_SPARKS:
          if (player_c.sprinting && !player_c.jumping && !model_c.invisible) {
            map["spawn_rate"] = "400.f";
          } else {
            map["spawn_rate"] = "0.f";
          }
          emitter_vel *= 0.5;
          break;
        case BoneEmitterType::HIT:
          if (current.emitted) {
            current.emitted = false;
            map["spawn_rate"] = "100000.f";
          } else {
            map["spawn_rate"] = "0.f";
          }
          break;
        case BoneEmitterType::SHOOT:
          if (current.emitted) {
            current.emitted = false;
            map["spawn_rate"] = "100000.f";
          } else {
            map["spawn_rate"] = "0.f";
          }
          break;
        case BoneEmitterType::GOAL_MAKER:
          if (current.emitted) {
            map["spawn_rate"] = "1000.f";
          } else {
            map["spawn_rate"] = "0.f";
          }
          break;
      }

      glob::SetEmitPosition(particle_c.handles[i], pos);
      glob::SetParticleDirection(particle_c.handles[i], vel);
      map["velocity"] = std::to_string(length(vel));
      map["emitter_vel"] = std::to_string(emitter_vel.x) + " " +
                           std::to_string(emitter_vel.y) + " " +
                           std::to_string(emitter_vel.z);

      glob::SetParticleSettings(particle_c.handles[i], map);
    }
  }

  auto view_particle = registry.view<ParticleComponent>();
  for (auto& entity : view_particle) {
    auto& particle_c = view_particle.get(entity);

    for (int i = 0; i < particle_c.handles.size(); ++i) {
      glob::UpdateParticles(particle_c.handles[i], dt);
    }
  }
}

void ParticleSystem::ReceiveGameEvent(const GameEvent& event) {
  switch (event.type) {
    case GameEvent::HIT: {
      HandleHit(event);
    } break;
    case GameEvent::SHOOT: {
      HandleShoot(event);
    } break;
    case GameEvent::GOAL: {
      HandleGoalMaker(event, true);
    } break;
    case GameEvent::RESET: {
      HandleGoalMaker(event, false);
    } break;
  }
}

void ParticleSystem::HandleHit(const GameEvent& event) {
  auto registry = engine_->GetCurrentRegistry();

  auto view_model_movable =
      registry->view<IDComponent, ParticleComponent, TransformComponent,
                     FollowBoneComponent, AnimationComponent, ModelComponent,
                     PlayerComponent>();
  for (auto& entity : view_model_movable) {
    auto& id_c = view_model_movable.get<IDComponent>(entity);
    auto& particle_c = view_model_movable.get<ParticleComponent>(entity);
    auto& transform_c = view_model_movable.get<TransformComponent>(entity);
    auto& follow_c = view_model_movable.get<FollowBoneComponent>(entity);
    auto& animation_c = view_model_movable.get<AnimationComponent>(entity);
    auto& model_c = view_model_movable.get<ModelComponent>(entity);
    auto& player_c = view_model_movable.get<PlayerComponent>(entity);

    if (id_c.id == event.kick.player_id) {
      PhysicsComponent* physics_c = nullptr;
      if (registry->has<PhysicsComponent>(entity)) {
        physics_c = &registry->get<PhysicsComponent>(entity);
      }

      if (animation_c.bone_transforms.empty()) {
        break;
      }
      for (int i = 0; i < follow_c.emitters.size(); ++i) {
        BoneEmitter& current = follow_c.emitters[i];
        if (current.type == BoneEmitterType::HIT) {
          int id = current.bone_id;
          auto anim_trans = animation_c.bone_transforms[id];

          const glm::mat4 pre_rotation =
              glm::rotate(glm::pi<float>() / 2.f, glm::vec3(0, 1, 0)) *
              glm::rotate(-glm::pi<float>() / 2.f, glm::vec3(1, 0, 0));
          glm::mat4 model_trans =
              glm::translate(transform_c.position) *
              glm::toMat4(transform_c.rotation + model_c.rot_offset) *
              glm::translate(-model_c.offset) * glm::scale(transform_c.scale);

          model_trans = model_trans * pre_rotation;

          glm::mat4 trans = model_trans * anim_trans;

          glm::mat3 normal_transform =
              glm::transpose(glm::inverse(glm::mat3(trans)));
          glm::vec3 local_direction = normalize(current.dir);
          glm::vec3 direction = normalize(normal_transform * local_direction);

          glm::vec3 pos = glm::vec3(trans * glm::vec4(current.pos, 1));

          glm::vec3 vel = current.speed * direction;

          glm::vec3 emitter_vel = glm::vec3(0);
          if (physics_c) {
            emitter_vel += physics_c->velocity;
          }
          std::unordered_map<std::string, std::string> map;
          switch (current.type) {
            case BoneEmitterType::HIT:
              map["spawn_rate"] = "100000.f";
              current.emitted = true;
              break;
          }

          glob::SetEmitPosition(particle_c.handles[i], pos);
          glob::SetParticleDirection(particle_c.handles[i], vel);
          map["velocity"] = std::to_string(length(vel));
          map["emitter_vel"] = std::to_string(emitter_vel.x) + " " +
                               std::to_string(emitter_vel.y) + " " +
                               std::to_string(emitter_vel.z);

          glob::SetParticleSettings(particle_c.handles[i], map);
        }
      }
      break;
    }
  }
}

void ParticleSystem::HandleShoot(const GameEvent& event) {
  auto registry = engine_->GetCurrentRegistry();

  auto view_model_movable =
      registry->view<IDComponent, ParticleComponent, TransformComponent,
                     FollowBoneComponent, AnimationComponent, ModelComponent,
                     PlayerComponent>();
  for (auto& entity : view_model_movable) {
    auto& id_c = view_model_movable.get<IDComponent>(entity);
    auto& particle_c = view_model_movable.get<ParticleComponent>(entity);
    auto& transform_c = view_model_movable.get<TransformComponent>(entity);
    auto& follow_c = view_model_movable.get<FollowBoneComponent>(entity);
    auto& animation_c = view_model_movable.get<AnimationComponent>(entity);
    auto& model_c = view_model_movable.get<ModelComponent>(entity);
    auto& player_c = view_model_movable.get<PlayerComponent>(entity);

    if (id_c.id == event.shoot.player_id) {
      PhysicsComponent* physics_c = nullptr;
      if (registry->has<PhysicsComponent>(entity)) {
        physics_c = &registry->get<PhysicsComponent>(entity);
      }

      if (animation_c.bone_transforms.empty()) {
        break;
      }
      for (int i = 0; i < follow_c.emitters.size(); ++i) {
        BoneEmitter& current = follow_c.emitters[i];
        if (current.type == BoneEmitterType::SHOOT) {
          int id = current.bone_id;
          auto anim_trans = animation_c.bone_transforms[id];

          const glm::mat4 pre_rotation =
              glm::rotate(glm::pi<float>() / 2.f, glm::vec3(0, 1, 0)) *
              glm::rotate(-glm::pi<float>() / 2.f, glm::vec3(1, 0, 0));
          glm::mat4 model_trans =
              glm::translate(transform_c.position) *
              glm::toMat4(transform_c.rotation + model_c.rot_offset) *
              glm::translate(-model_c.offset) * glm::scale(transform_c.scale);

          model_trans = model_trans * pre_rotation;

          glm::mat4 trans = model_trans * anim_trans;

          glm::mat3 normal_transform =
              glm::transpose(glm::inverse(glm::mat3(trans)));
          glm::vec3 local_direction = normalize(current.dir);
          glm::vec3 direction = normalize(normal_transform * local_direction);

          glm::vec3 pos = glm::vec3(trans * glm::vec4(current.pos, 1));

          glm::vec3 vel = current.speed * direction;

          glm::vec3 emitter_vel = glm::vec3(0);
          if (physics_c) {
            emitter_vel += physics_c->velocity;
          }
          std::unordered_map<std::string, std::string> map;
          switch (current.type) {
            case BoneEmitterType::SHOOT:
              map["spawn_rate"] = "100000.f";
              current.emitted = true;
              break;
          }

          glob::SetEmitPosition(particle_c.handles[i], pos);
          glob::SetParticleDirection(particle_c.handles[i], vel);
          map["velocity"] = std::to_string(length(vel));
          map["emitter_vel"] = std::to_string(emitter_vel.x) + " " +
                               std::to_string(emitter_vel.y) + " " +
                               std::to_string(emitter_vel.z);

          glob::SetParticleSettings(particle_c.handles[i], map);
        }
      }
      break;
    }
  }
}

void ParticleSystem::HandleGoalMaker(const GameEvent& event, bool emit) {
  auto registry = engine_->GetCurrentRegistry();

  auto view_model_movable =
      registry->view<IDComponent, ParticleComponent, TransformComponent,
                     FollowBoneComponent, AnimationComponent, ModelComponent,
                     PlayerComponent>();
  for (auto& entity : view_model_movable) {
    auto& id_c = view_model_movable.get<IDComponent>(entity);
    auto& particle_c = view_model_movable.get<ParticleComponent>(entity);
    auto& transform_c = view_model_movable.get<TransformComponent>(entity);
    auto& follow_c = view_model_movable.get<FollowBoneComponent>(entity);
    auto& animation_c = view_model_movable.get<AnimationComponent>(entity);
    auto& model_c = view_model_movable.get<ModelComponent>(entity);
    auto& player_c = view_model_movable.get<PlayerComponent>(entity);

    if (id_c.id == event.goal.goal_maker || event.type == GameEvent::RESET) {
      PhysicsComponent* physics_c = nullptr;
      if (registry->has<PhysicsComponent>(entity)) {
        physics_c = &registry->get<PhysicsComponent>(entity);
      }

      if (animation_c.bone_transforms.empty()) {
        break;
      }
      for (int i = 0; i < follow_c.emitters.size(); ++i) {
        BoneEmitter& current = follow_c.emitters[i];
        if (current.type == BoneEmitterType::GOAL_MAKER) {
          int id = current.bone_id;
          auto anim_trans = animation_c.bone_transforms[id];

          const glm::mat4 pre_rotation =
              glm::rotate(glm::pi<float>() / 2.f, glm::vec3(0, 1, 0)) *
              glm::rotate(-glm::pi<float>() / 2.f, glm::vec3(1, 0, 0));
          glm::mat4 model_trans =
              glm::translate(transform_c.position) *
              glm::toMat4(transform_c.rotation + model_c.rot_offset) *
              glm::translate(-model_c.offset) * glm::scale(transform_c.scale);

          model_trans = model_trans * pre_rotation;

          glm::mat4 trans = model_trans * anim_trans;

          glm::mat3 normal_transform =
              glm::transpose(glm::inverse(glm::mat3(trans)));
          glm::vec3 local_direction = normalize(current.dir);
          glm::vec3 direction = normalize(normal_transform * local_direction);

          glm::vec3 pos = glm::vec3(trans * glm::vec4(current.pos, 1));

          glm::vec3 vel = current.speed * direction;

          glm::vec3 emitter_vel = glm::vec3(0);
          if (physics_c) {
            emitter_vel += physics_c->velocity;
          }
          std::unordered_map<std::string, std::string> map;
          switch (current.type) {
            case BoneEmitterType::GOAL_MAKER:
              map["spawn_rate"] = "1000.f";
              current.emitted = emit;
              break;
          }

          glob::SetEmitPosition(particle_c.handles[i], pos);
          glob::SetParticleDirection(particle_c.handles[i], vel);
          map["velocity"] = std::to_string(length(vel));
          map["emitter_vel"] = std::to_string(emitter_vel.x) + " " +
                               std::to_string(emitter_vel.y) + " " +
                               std::to_string(emitter_vel.z);

          glob::SetParticleSettings(particle_c.handles[i], map);
        }
      }
      if (event.type != GameEvent::RESET) {
        break;
      }
    }
  }
}
