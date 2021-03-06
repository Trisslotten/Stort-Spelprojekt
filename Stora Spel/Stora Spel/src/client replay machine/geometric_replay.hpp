#ifndef GEOMETRIC_REPLAY_HPP_
#define GEOMETRIC_REPLAY_HPP_

#include <string>
#include <vector>

#include <entity/registry.hpp>
#include <shared/id_component.hpp>

#include "data_frame.hpp"

class Engine;

enum ReplayObjectType {
  REPLAY_PLAYER = 0,  // Start
  REPLAY_BALL,
  REPLAY_PICKUP,
  REPLAY_WALL,
  REPLAY_SHOT,
  REPLAY_TELEPORT_SHOT,
  REPLAY_MISSILE,
  REPLAY_FORCE_PUSH,
  REPLAY_MINE,
  REPLAY_BLACKHOLE,
  REPLAY_HOOK,
  NUM_OF_REPLAY_OBJECT_TYPES  // End
};

class GeometricReplay {
 private:
  struct ChannelEntry {
    unsigned int frame_number = -1;
    DataFrame* data_ptr = nullptr;
    bool ending_entry = false;

    // Default constructor
    ChannelEntry() {}

    // Copy Constructor
    ChannelEntry(const ChannelEntry& in_ce) {
      if (data_ptr != nullptr) {
        delete data_ptr;
      }

      this->frame_number = in_ce.frame_number;
      this->data_ptr = nullptr;
      if (in_ce.data_ptr != nullptr) {
        this->data_ptr = in_ce.data_ptr->Clone();
      }
      this->ending_entry = in_ce.ending_entry;
    }

    // Destructor
    ~ChannelEntry() {
      if (data_ptr != nullptr) {
        delete data_ptr;
      }
    }

    // Assignment operator
    ChannelEntry& operator=(ChannelEntry const& rhs) {
      if (&rhs == this) return *this;

      if (data_ptr != nullptr) {
        delete data_ptr;
      }

      this->frame_number = rhs.frame_number;
      this->data_ptr = nullptr;
      if (rhs.data_ptr != nullptr) {
        this->data_ptr = rhs.data_ptr->Clone();
      }
      this->ending_entry = rhs.ending_entry;

      return *this;
    }
  };

  struct FrameChannel {
    ReplayObjectType object_type;
    EntityID object_id;
    std::vector<ChannelEntry> entries;
    unsigned int index_a = 0;
    unsigned int index_b = 0;

    // Destructor
    ~FrameChannel() { entries.clear(); }

    // Assignment operator
    FrameChannel& operator=(FrameChannel const& rhs) {
      if (&rhs == this) return *this;

      this->object_type = rhs.object_type;
      this->object_id = rhs.object_id;

      this->entries = rhs.entries;

      this->index_a = rhs.index_a;
      this->index_b = rhs.index_b;

      return *this;
    }
  };

  struct CapturedGameEvent {
    GameEvent event;
    unsigned int frame_number;
  };

  struct StateLogEntry {
    bool blackout_active = false;
    bool goals_switched = false;
    unsigned int frame_number;
  };

  //---
  ReplayObjectType IdentifyEntity(entt::entity& in_entity,
                                  entt::registry& in_registry);

  void FillChannelEntry(ChannelEntry& in_ce, entt::entity& in_entity,
                        entt::registry& in_registry);
  DataFrame* PolymorphIntoDataFrame(entt::entity& in_entity,
                                    entt::registry& in_registry);

  DataFrame* InterpolateDataFrame(unsigned int in_channel_index,
                                  unsigned int in_entry_index_a,
                                  unsigned int in_entry_index_b,
                                  unsigned int in_target_frame_num);

  void InterpolateEntityData(unsigned int in_channel_index,
                             entt::entity& in_entity,
                             entt::registry& in_registry);
  void DepolymorphFromDataframe(DataFrame* in_df_ptr, ReplayObjectType in_type,
                                entt::entity& in_entity,
                                entt::registry& in_registry);

  void CreateEntityFromChannel(unsigned int in_channel_index,
                               entt::registry& in_registry);

  void CreateChannelForEntity(entt::entity& in_entity, IDComponent& in_id_c,
                              entt::registry& in_registry);
  void FrameChannelCleanUp();

 protected:
  std::vector<FrameChannel> channels_;
  unsigned int threshhold_age_;
  unsigned int current_frame_number_write_ = 0;
  unsigned int current_frame_number_read_ = 0;

  Engine* engine_;
  std::vector<CapturedGameEvent> captured_events_;
  unsigned int next_event_index_to_read_ = 0;

  std::vector<StateLogEntry> state_logs_;

  GeometricReplay();

 public:
  GeometricReplay(unsigned int in_replay_length_sec,
                  unsigned int in_frames_per_sec);
  ~GeometricReplay();

  // NTS:	Delete copy constructor
  //		and assignment operator
  GeometricReplay(GeometricReplay&) = delete;
  void operator=(GeometricReplay const&) = delete;

  GeometricReplay* Clone();

  bool SaveFrame(entt::registry& in_registry);
  bool LoadFrame(entt::registry& in_registry);

  void SetEndingFrame(EntityID in_id, entt::registry& in_registry);

  void SetReadFrameToStart();

  void ChannelCatchUp();

  void ReceiveGameEvent(GameEvent event);
  void SetEngine(Engine* eng) { engine_ = eng; }

  void LogCurrentState();
  void LogReset();
  int GetStartingEnvironment();

  void ClearAllVectors();

  //---

  std::string GetGeometricReplayTree();
  std::string GetGeometricReplayState();
  std::string GetGeometricReplaySummary();
  std::string GetGeometricReplayEventList();
};

#endif  // !GEOMETRIC_REPLAY_HPP_
