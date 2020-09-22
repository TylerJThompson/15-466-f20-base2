#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//function to change state of rgb:
	void change_rgb_state();

	//variable for rgb state puzzle
	float red_state = 0.0f;
	float green_state = 0.0f;
	float blue_state = 0.0f;
	Uint8 total_presses = 0;
	Uint8 red_presses = 0;
	Uint8 green_presses = 0;
	Uint8 blue_presses = 0;

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space;

	//bounds on where the player can move:
	struct Bounds {
		float x_min = -0.9f;
		float x_max = 0.9f;
		float y_min = -0.9f;
		float y_max = 0.9f;
	} bounds;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//button stands:
	Scene::Transform *red_stand = nullptr;
	Scene::Transform *green_stand = nullptr;
	Scene::Transform *blue_stand = nullptr;

	//buttons:
	Scene::Transform *red_button = nullptr;
	Scene::Transform *green_button = nullptr;
	Scene::Transform *blue_button = nullptr;
	Scene::Transform *close_button = nullptr;

	//for helping if button is moving:
	bool moving_down = false;
	bool moving_up = false;
	float max_button_z = 0.0f;

	//roof (root) of player's building:
	Scene::Transform *player_roof = nullptr;

	//to move the building down at the end of the game:
	bool building_moving = false;
	
	//player:
	Scene::Transform *player = nullptr;

	//camera:
	Scene::Camera *camera = nullptr;

	//text written on screen
	std::string bottom_text = "Mouse motion rotates camera; WASD moves; escape ungrabs mouse";
	std::string top_text = "RGB: 0.0, 0.0, 0.0    Total Presses: 0, Red Presses: 0, Green Presses: 0, Blue Presses: 0";
};
