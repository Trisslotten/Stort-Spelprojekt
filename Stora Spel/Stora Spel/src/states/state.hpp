#ifndef STATE_HPP_
#define STATE_HPP_

#define LATENCY 0.006f

#include <list>
#include <NetAPI\packet.hpp>
#include <entt.hpp>
#include <glm/glm.hpp>
#include <glob/graphics.hpp>
#include "Chat.hpp"
#include "shared/shared.hpp"
#include <ecs/components/button_component.hpp>
#include <playerdata.hpp>

class Engine;

enum class StateType {
  MAIN_MENU,
  CONNECT_MENU,
  LOBBY,
  PLAY,
};

class State {
 public:
  // when program starts
  virtual void Startup() = 0;

  // when state changed to this state
  virtual void Init() = 0;

  virtual void Update(float dt) = 0;

  virtual void UpdateNetwork() = 0;

  // when state becomes inactive
  virtual void Cleanup() = 0;

  // the type of state
  virtual StateType Type() = 0;

  void SetEngine(Engine* engine) { engine_ = engine; }

  State() = default;
  ~State() {}

 protected:
  Engine* engine_ = nullptr;

 private:
};

/////////////////////// MAIN MENU ///////////////////////

class MainMenuState : public State {
 public:
  void Startup() override;
  void Init() override;
  void Update(float dt) override;
  void UpdateNetwork() override;
  void Cleanup() override;

  StateType Type() { return StateType::MAIN_MENU; }

 private:
  void CreateMainMenu();
  void CreateSettingsMenu();

  entt::registry registry_mainmenu_;
  entt::registry registry_settings_;

  glob::Font2DHandle font_test_ = 0;

  // Inherited via State
};

/////////////////////// LOBBY ///////////////////////

struct LobbyPlayer {
  unsigned int team;
  bool ready;
};

class LobbyState : public State {
 public:
  void Startup() override;
  void Init() override;
  void Update(float dt) override;
  void UpdateNetwork() override;
  void Cleanup() override;

  StateType Type() { return StateType::LOBBY; }

  void HandleUpdateLobbyTeamPacket(NetAPI::Common::Packet& packet);
  void SetMyId(int client_id) { my_id_ = client_id; }

 private:
  entt::registry registry_lobby_;
  void CreateBackgroundEntities();
  void CreateGUIElements();
  void DrawTeamSelect();

  glob::GUIHandle team_select_back_;
  glob::Font2DHandle font_team_names_;
  std::unordered_map<int, LobbyPlayer> lobby_players_;

  void ReadyButtonFunc();
  ButtonComponent* ready_button_c = nullptr;
  bool me_ready_ = false;

  void SendJoinTeam(unsigned int team);
  int my_id_ = 0;
};
/////////////////////// ConnectMenuState
class ConnectMenuState : public State {
 public:
  void Startup() override;
  void Init() override;
  void Update(float dt) override;
  void UpdateNetwork() override;
  void Cleanup() override;
  StateType Type() { return StateType::CONNECT_MENU; }

 private:
  struct InputField {
    InputField(){};
    InputField(glm::vec2 in_size, glm::vec2 in_pos,
               std::string initial_text = "") {
      size = in_size;
      pos = in_pos;
    }
    bool focus = false;
    std::string input_field = "";
    glm::vec2 size = glm::vec2(0.0f, 0.0f);
    glm::vec2 pos = glm::vec2(0.0f, 0.0f);
    glob::GUIHandle hndl = 0;
  };
  std::string ip_ = "localhost";
  std::string port_ = "1337";
  InputField ip_field_;
  InputField port_field_;
  glob::Font2DHandle font_test_ = 0;
  entt::registry registry_connect_menu_;
};
/////////////////////// PLAY ///////////////////////

class PlayState : public State {
 public:
  void Startup() override;
  void Init() override;
  void Update(float dt) override;
  void UpdateNetwork() override;
  void Cleanup() override;

  StateType Type() { return StateType::PLAY; }

  void SetEntityTransform(EntityID player_id, glm::vec3 pos,
                          glm::quat orientation);
  void SetCameraOrientation(glm::quat orientation);
  void SetEntityIDs(std::vector<EntityID> player_ids, EntityID my_id,
                    EntityID ball_id) {
    player_ids_ = player_ids;
    my_id_ = my_id;
    ball_id_ = ball_id;
  }
  void SetCurrentStamina(float stamina) { current_stamina_ = stamina; }

  void CreatePickUp(glm::vec3 position);
  void CreateCannonBall(EntityID id);
  void CreateForcePushObject(EntityID id);
  void DestroyEntity(EntityID id);
  void SwitchGoals();

  void SetKeyBinds(std::unordered_map<int, int>* keybinds) {keybinds_ = keybinds;}
  void OnServerFrame();

 private:
  void CreateInitialEntities();
  void CreatePlayerEntities();
  void CreateArenaEntity();
  void CreateBallEntity();
  void CreateInGameMenu();

  void TestCreateLights();

  void ToggleInGameMenu();
  void UpdateInGameMenu(bool show_menu);
  FrameState SimulateMovement(std::vector<int> *action, float dt);
  void MovePlayer(float dt);
  
  ////////////////////////////////////////

  entt::registry registry_gameplay_;

  std::vector<EntityID> player_ids_;
  EntityID my_id_, ball_id_;
  float current_stamina_ = 0.f;

  std::unordered_map<EntityID, std::pair<glm::vec3, glm::quat>> transforms_;
  std::unordered_map<EntityID, std::pair<glm::vec3, glm::quat>> new_transforms_;
  glm::vec3 player_new_pos_;
  glm::quat player_new_rotation_;
  entt::entity my_entity_;

  std::unordered_map<int, int>* keybinds_;
  entt::entity blue_goal_light_;
  entt::entity red_goal_light_;

  glob::Font2DHandle font_test_ = 0;
  glob::E2DHandle e2D_test_, e2D_test2_;
  glob::GUIHandle in_game_menu_gui_ = 0;
  glob::GUIHandle gui_test_, gui_teamscore_, gui_stamina_base_,
      gui_stamina_fill_, gui_stamina_icon_, gui_quickslots_;

  bool show_in_game_menu_buttons_ = false;
  std::list<PlayerData> history_;
  FrameState predicted_state_;
  float history_duration_;
};

#endif  // STATE_HPP_