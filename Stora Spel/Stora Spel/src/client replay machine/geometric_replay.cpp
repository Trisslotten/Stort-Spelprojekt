#include "geometric_replay.hpp"

#include <map>

#include <ecs/components/ball_component.hpp>
#include <ecs/components/player_component.hpp>
#include <shared/transform_component.hpp>
#include <util/global_settings.hpp>
// Private---------------------------------------------------------------------

void GeometricReplay::FillChannelEntry(ChannelEntry& in_ce,
                                       entt::entity& in_entity,
                                       entt::registry& in_registry) {
  // Channel entry frame
  in_ce.frame_number = this->current_frame_number_write_;

  // Channel entry data
  DataFrame* df_ptr = this->PolymorphIntoDataFrame(in_entity, in_registry);

  // Then save
  in_ce.data_ptr = df_ptr;

  // Channel entry is ending entry?
  // Here: No
  in_ce.ending_entry = false;
}

DataFrame* GeometricReplay::PolymorphIntoDataFrame(
    entt::entity& in_entity, entt::registry& in_registry) {
  DataFrame* ret_ptr = nullptr;
  // Start by identifying what kind of entity we
  // are trying to save
  if (in_registry.has<PlayerComponent>(in_entity)) {
    // If there is a player component we know it is
    // a player avatar

    // WIP: Might be needed for animations
    // PlayerComponent& player_c = in_registry.get<PlayerComponent>(in_entity);

    TransformComponent& transform_c =
        in_registry.get<TransformComponent>(in_entity);

    ret_ptr = new PlayerFrame(transform_c.position, transform_c.rotation,
                              transform_c.scale);

  } else if (in_registry.has<BallComponent>(in_entity)) {
    // Otherwise, if there is a ball component we
    // know it to be a ball
    TransformComponent& transform_c =
        in_registry.get<TransformComponent>(in_entity);

    ret_ptr = new BallFrame(transform_c.position, transform_c.rotation,
                            transform_c.scale);
  }
  return ret_ptr;
}

void GeometricReplay::InterpolateEntityData(unsigned int in_channel_index,
                                            entt::entity& in_entity,
                                            entt::registry& in_registry) {
  // Check the current reading frame and determine the indices
  // for interpolation points 'a' & 'b' in the FrameChannel
  unsigned int& c = this->current_frame_number_read_;
  unsigned int& a = this->channels_.at(in_channel_index).index_a;
  unsigned int& b = this->channels_.at(in_channel_index).index_b;

  // If the current frame is at or has surpassed the frame at point 'b'
  if (c >= this->channels_.at(in_channel_index).entries.at(b).frame_number) {
    // Move 'a' forward to 'b'
    // Note that we move 'a' to 'b', and do not increment it
    // This is to prevent 'a' going out of scope
    a = b;
    // Move 'b' forward to the next index if it won't go
    // over the channel's last entry
    if (b + 1 != this->channels_.at(in_channel_index).entries.size()) {
      b++;
    }
  }

  /*
        WIP:
        If 'a' == the index of the last entry
        no interpolation needs to be done,
        we just get the data from the last frame
  */

  // Get the DataFrame at 'a' and interpolate it forwards
  // towards 'b' to find the DataFrame representing the current frame
  unsigned int dist_to_target =
      this->current_frame_number_read_ -
      this->channels_.at(in_channel_index).entries.at(a).frame_number;
  unsigned int dist_to_b =
      this->channels_.at(in_channel_index).entries.at(b).frame_number -
      this->channels_.at(in_channel_index).entries.at(a).frame_number;

  // If the distance to the target is greater than or equal to the
  // distance to 'b' we have passed the last entry in the channel
  if (dist_to_target >= dist_to_b) {
	  //We set the distance to the target to be the distance
	  // to 'b', saying that all frames past the last entry
	  // has the value of the last entry
    dist_to_target = dist_to_b;
  }

  DataFrame* df_a_ptr =
      this->channels_.at(in_channel_index).entries.at(a).data_ptr;
  DataFrame* df_b_ptr =
      this->channels_.at(in_channel_index).entries.at(b).data_ptr;

  DataFrame* df_c_ptr =
      df_a_ptr->InterpolateForward(dist_to_target, dist_to_b, (*df_b_ptr));
  // NTS:	InterpolateForward allocates new memory space
  //		Remember to clean up!

  // Load the data from the created frame into the
  // components of the entity
  this->DepolymorphFromDataframe(df_c_ptr, in_entity, in_registry);

  // De-allocate and clean up after InterpolateForward()
  delete df_c_ptr;
}

void DepolymorphFromDataframe(DataFrame* in_df_ptr, entt::entity& in_entity,
                              entt::registry& in_registry) {
  switch (in_df_ptr->GetFrameType()) {
    case FRAME_PLAYER:
      // Cast
      PlayerFrame* pf_c_ptr = dynamic_cast<PlayerFrame*>(in_df_ptr);
      // Get
      TransformComponent& transform_c =
          in_registry.get<TransformComponent>(in_entity);
      // Transfer
      transform_c.position = pf_c_ptr->position_;
      transform_c.rotation = pf_c_ptr->rotation_;
      transform_c.scale = pf_c_ptr->scale_;
      //
      // WIP: Handle model and animation components
      //
      break;
    case FRAME_BALL:
      // Cast
      BallFrame* pf_c_ptr = dynamic_cast<BallFrame*>(in_df_ptr);
      // Get
      TransformComponent& transform_c =
          in_registry.get<TransformComponent>(in_entity);
      // Transfer
      transform_c.position = pf_c_ptr->position_;
      transform_c.rotation = pf_c_ptr->rotation_;
      transform_c.scale = pf_c_ptr->scale_;
      //
      // WIP: Handle model component
      //
      break;
    default:
      GlobalSettings::Access()->WriteError(__FILE__, __FUNCTION__,
                                           "Unknown FrameType");
      break;
  }
}

void GeometricReplay::CreateEntityFromChannel(unsigned int in_channel_index,
                                              entt::registry& in_registry) {
  // Get the first frame of the channel that is tracking the entity
  DataFrame* df_ptr =
      this->channels_.at(in_channel_index).entries.at(0).data_ptr;

  // Fetch FrameType
  FrameType ft = df_ptr->GetFrameType();

  // Create base entity
  entt::entity entity = in_registry.create();

  // Switch-case over FrameType:s
  // In each:
  //	- Read data and assign components for that entity type
  switch (ft) {
    case FRAME_PLAYER:
      PlayerFrame* pf_ptr = dynamic_cast<PlayerFrame*>(df_ptr);
      in_registry.assign<IDComponent>(
          entity, this->channels_.at(in_channel_index).object_id);
      in_registry.assign<TransformComponent>(entity, pf_ptr->position_,
                                             pf_ptr->rotation_, pf_ptr->scale_);
      //
      // WIP: Handle model and animation components
      //

      break;
    case FRAME_BALL:
      BallFrame* bf_ptr = dynamic_cast<BallFrame*>(df_ptr);
      in_registry.assign<IDComponent>(
          entity, this->channels_.at(in_channel_index).object_id);
      in_registry.assign<TransformComponent>(entity, pf_ptr->position_,
                                             pf_ptr->rotation_, pf_ptr->scale_);

      //
      // WIP: Handle model component
      //

      break;
    default:
      GlobalSettings::Access()->WriteError(__FILE__, __FUNCTION__,
                                           "Unknown FrameType");
      break;
  }
}

// Public----------------------------------------------------------------------

GeometricReplay::GeometricReplay(unsigned int in_length_sec,
                                 unsigned int in_frames_per_sec) {
  this->threshhold_age_ = in_length_sec * in_frames_per_sec;
}

GeometricReplay::~GeometricReplay() {}

bool GeometricReplay::SaveFrame(entt::registry& in_registry) {
  // Loop over all entries with an id component
  entt::basic_view view = in_registry.view<IDComponent>();
  for (entt::entity entity : view) {
    IDComponent& id_c = in_registry.get<IDComponent>(entity);

    // Check if entity with that id has a channel
    bool id_unfound = true;
    for (unsigned int i = 0; i < this->channels_.size && id_unfound; i++) {
      if (id_c.id == this->channels_.at(i).object_id) {
        id_unfound = false;
        // If it does we check if a new interpolation point should be added
        // dependent on the object's type
        DataFrame* temp_df = nullptr;
        if (in_registry.has<PlayerComponent>(entity)) {
          // PLAYER CASE
          TransformComponent& transform_c =
              in_registry.get<TransformComponent>(entity);
          temp_df = new PlayerFrame(transform_c.position, transform_c.rotation,
                                    transform_c.scale);
        } else if (in_registry.has<PlayerComponent>(entity)) {
          // BALL CASE
          TransformComponent& transform_c =
              in_registry.get<TransformComponent>(entity);
          temp_df = new BallFrame(transform_c.position, transform_c.rotation,
                                  transform_c.scale);
        }

        // Compare last entry to what would be the current frame's
        if ((temp_df != nullptr) &&
            this->channels_.at(i).entries.back().data_ptr->ThresholdCheck(
                *temp_df)) {
          // IF true, save DataFrame
          ChannelEntry temp_ce;
          temp_ce.frame_number = this->current_frame_number_write_;
          temp_ce.data_ptr = temp_df;
          temp_ce.ending_entry = false;
          this->channels_.at(i).entries.push_back(temp_ce);
        }
      }
    }

    // If after looping through an object still hasn't been found
    // it should be added to its own channel
    if (id_unfound) {
      // Entry
      ChannelEntry temp_ce;
      this->FillChannelEntry(temp_ce, entity, in_registry);

      // Channel
      FrameChannel temp_fc;
      temp_fc.object_id = id_c.id;
      temp_fc.entries.push_back(temp_ce);

      this->channels_.push_back(temp_fc);
    }
  }

  // Check the first entry in each channel
  // Its age can be calculated by checking the current number of
  // frames passed and comparing it to the frame it was saved on
  // We do not want to record the entire match (due to memory consumption)
  // so therefore:
  //	1. Check if there are more than one entry in channel
  //	2. If so, check if second entry is over age threshold
  //	3. If so, remove first entry in channel OR remove entire
  //	channel if the second entry marks an ending entry*
  //
  // *When an object disappears from the game world (and thus the replay)
  // register that as an "ending entry".
  for (unsigned int i = 0; i < this->channels_.size(); i++) {
    if (this->channels_.at(i).entries.size() > 1) {  //(1.)
      unsigned int age = this->current_frame_number_write_ -
                         this->channels_.at(i).entries.at(1).frame_number;
      if (age > this->threshhold_age_) {                         //(2.)
        if (this->channels_.at(i).entries.at(1).ending_entry) {  //(3.)
          this->channels_.erase(this->channels_.begin() + i,
                                this->channels_.begin + i);
          i--;
        } else {
          this->channels_.at(i).entries.erase(
              this->channels_.at(i).entries.begin(),
              this->channels_.at(i).entries.begin());
        }
      }
    }
  }

  // After having saved a frame, increment
  this->current_frame_number_write_++;

  return false;
}

bool GeometricReplay::LoadFrame(entt::registry& in_registry) {
  // Get all entities with an IDComponent
  entt::basic_view view = in_registry.view<IDComponent>();

  // Loop over all channels
  for (unsigned int i = 0; i < this->channels_.size(); i++) {
    // Try finding the current channel's ID in the view
    bool id_unfound = true;

    for (entt::entity entity : view) {
      IDComponent& id_c = in_registry.get<IDComponent>(entity);

      if (this->channels_.at(i).object_id == id_c.id) {
        // If the id is found set the entity's
        // component data using the interpolation function
        this->InterpolateEntityData(i, entity, in_registry);

        // Note that we found id and jump out of for-loop
        id_unfound = false;
        break;
      }
    }

    if (id_unfound) {
      // Create the entity that existed in the replay
      // but not in the registry
      this->CreateEntityFromChannel(i, in_registry);
    }
  }

  return false;
}