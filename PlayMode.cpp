#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint city_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > city_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("city.pnct"));
	city_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > city_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("city.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		
		Mesh const &mesh = city_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = city_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*city_scene) {
	//get pointers to stands, buttons, and player for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Stand.001") red_stand = &transform;
		else if (transform.name == "Stand.002") green_stand = &transform;
		else if (transform.name == "Stand.003") blue_stand = &transform;
		else if (transform.name == "Button.001") red_button = &transform;
		else if (transform.name == "Button.002") green_button = &transform;
		else if (transform.name == "Button.003") blue_button = &transform;
		else if (transform.name == "Roof.001") player_roof = &transform;
		else if (transform.name == "Player") player = &transform;
	}
	if (red_stand == nullptr) throw std::runtime_error("Red stand not found.");
	if (green_stand == nullptr) throw std::runtime_error("Green stand not found.");
	if (blue_stand == nullptr) throw std::runtime_error("Blue stand not found.");
	if (red_button == nullptr) throw std::runtime_error("Red button not found.");
	if (green_button == nullptr) throw std::runtime_error("Green button not found.");
	if (blue_button == nullptr) throw std::runtime_error("Blue button not found.");
	if (player_roof == nullptr) throw std::runtime_error("Player roof not found.");
	if (player == nullptr) throw std::runtime_error("Player not found.");

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.downs += 1;
			space.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			//set the button to start moving if it is close enough and isn't already moving
			if (!(close_button == nullptr || moving_down || moving_up || building_moving)) {
				moving_down = true;
				max_button_z = close_button->position.z;
			}
			space.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			//rotate the player only on the z-axis:
			player->rotation = glm::normalize(
				player->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, -1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(0.0f, -1.0f, 0.0f))
			);
			return true;
		}
	}
	return false;
}

void PlayMode::update(float elapsed) {
	//press selected button:
	if (moving_down) {
		close_button->position.z -= elapsed / 10.0f;
		close_button->position.z = std::max(close_button->position.z, max_button_z - 0.1f);
		if (close_button->position.z == max_button_z - 0.1f) {
			moving_down = false;
			moving_up = true;
			change_rgb_state();
		}
	} else if (moving_up) {
		close_button->position.z += elapsed / 10.0f;
		close_button->position.z = std::min(close_button->position.z, max_button_z);
		if (close_button->position.z == max_button_z) {
			moving_up = false;
			if (red_state == 1.0f && green_state == 1.0f && blue_state == 1.0f)
				building_moving = true;
		}
	}

	//game is over, building is lowering
	if (building_moving) {
		player_roof->position.z -= elapsed / 2.5f;
		player_roof->position.z = std::max(player_roof->position.z, -0.5f);
		if (player_roof->position.z < 0.0f && player_roof->position.z > -0.5f)
			player->position.z += elapsed / 2.5f;
	}

	//move player:
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = -10.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x = 1.0f;
		if (!left.pressed && right.pressed) move.x = -1.0f;
		if (down.pressed && !up.pressed) move.y = -1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = player->make_local_to_parent();
		glm::vec3 right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 forward = -frame[2];

		//make it so the player only moves in the xy-plane and stays within the current player bounds:
		float cam_position_z = player->position.z;
		glm::vec3 new_cam_position = player->position + (move.x * right + move.y * forward);
		new_cam_position.x = glm::min(bounds.x_max, glm::max(bounds.x_min, new_cam_position.x));
		new_cam_position.y = glm::min(bounds.y_max, glm::max(bounds.y_min, new_cam_position.y));
		new_cam_position.z = cam_position_z;

		//do not let player walk inside the red stand:
		if (new_cam_position.x >= red_stand->position.x - 0.1f && new_cam_position.x <= red_stand->position.x + 0.1f &&
			new_cam_position.y >= red_stand->position.y - 0.1f && new_cam_position.y <= red_stand->position.y + 0.1f)
			new_cam_position = player->position;

		//do not let player walk inside the green stand:
		if (new_cam_position.x >= green_stand->position.x - 0.1f && new_cam_position.x <= green_stand->position.x + 0.1f &&
			new_cam_position.y >= green_stand->position.y - 0.1f && new_cam_position.y <= green_stand->position.y + 0.1f)
			new_cam_position = player->position;

		//do not let player walk inside the blue stand:
		if (new_cam_position.x >= blue_stand->position.x - 0.1f && new_cam_position.x <= blue_stand->position.x + 0.1f &&
			new_cam_position.y >= blue_stand->position.y - 0.1f && new_cam_position.y <= blue_stand->position.y + 0.1f)
			new_cam_position = player->position;

		//check if in range of the buttons for pressing:
		//man, I can't remember the distance formula w/o help, this shouldn't be hard: https://www.khanacademy.org/math/geometry/hs-geo-analytic-geometry/hs-geo-distance-and-midpoints/v/distance-formula
		if (!(moving_down || moving_up || building_moving)) {
			if (std::sqrt(std::pow(player->position.x - red_stand->position.x, 2) + std::pow(player->position.y - red_stand->position.y, 2)) <= 0.35f) {
				bottom_text = "Hit the space bar to press the red button";
				close_button = red_button;
			}
			else if (std::sqrt(std::pow(player->position.x - green_stand->position.x, 2) + std::pow(player->position.y - green_stand->position.y, 2)) <= 0.35f) {
				bottom_text = "Hit the space bar to press the green button";
				close_button = green_button;
			}
			else if (std::sqrt(std::pow(player->position.x - blue_stand->position.x, 2) + std::pow(player->position.y - blue_stand->position.y, 2)) <= 0.35f) {
				bottom_text = "Hit the space bar to press the blue button";
				close_button = blue_button;
			}
			else {
				bottom_text = "Mouse motion rotates camera; WASD moves; escape ungrabs mouse";
				close_button = nullptr;
			}
		} else
			bottom_text = "Mouse motion rotates camera; WASD moves; escape ungrabs mouse";
			
		player->position = new_cam_position;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	GL_ERRORS();
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	GL_ERRORS();
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	GL_ERRORS();
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text(bottom_text,
			glm::vec3(-aspect + 0.5f * H, -1.0f + 0.5f * H, 0.0f),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(bottom_text,
			glm::vec3(-aspect + 0.5f * H + ofs, -1.0f + 0.5f * H + ofs, 0.0f),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		lines.draw_text(top_text,
			glm::vec3(-aspect + 0.5f * H, 1.0 - 1.5f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		ofs = 2.0f / drawable_size.y;
		lines.draw_text(top_text,
			glm::vec3(-aspect + 0.5f * H + ofs, 1.0 - 1.5f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}

void PlayMode::change_rgb_state() {
	if (close_button == red_button) {
		if (red_presses % 2 == 0) {
			red_state = glm::min(red_state + 1.0f, 1.0f);
			if (total_presses % 3 == 1)
				green_state = glm::max(green_state - 0.5f, 0.0f);
			else if (total_presses % 3 == 2)
				blue_state = glm::max(blue_state - 0.5f, 0.0f);
		} else {
			red_state = glm::max(red_state - 1.0f, 0.0f);
			if (total_presses % 3 == 1)
				green_state = glm::min(green_state + 0.5f, 1.0f);
			else if (total_presses % 3 == 2)
				blue_state = glm::min(blue_state + 0.5f, 1.0f);
		}
		red_presses += 1;
	} else if (close_button == green_button) {
		if (green_presses % 2 == 0) {
			green_state = glm::min(green_state + 1.0f, 1.0f);
			if (total_presses % 3 == 1)
				blue_state = glm::max(blue_state - 0.5f, 0.0f);
			else if (total_presses % 3 == 2)
				red_state = glm::max(red_state - 0.5f, 0.0f);
		}
		else {
			green_state = glm::max(green_state - 1.0f, 0.0f);
			if (total_presses % 3 == 1)
				blue_state = glm::min(blue_state + 0.5f, 1.0f);
			else if (total_presses % 3 == 2)
				red_state = glm::min(red_state + 0.5f, 1.0f);
		}
		green_presses += 1;
	} else if (close_button == blue_button) {
		if (blue_presses % 2 == 0) {
			blue_state = glm::min(blue_state + 1.0f, 1.0f);
			if (total_presses % 3 == 1)
				red_state = glm::max(red_state - 0.5f, 0.0f);
			else if (total_presses % 3 == 2)
				green_state = glm::max(green_state - 0.5f, 0.0f);
		}
		else {
			blue_state = glm::max(blue_state - 1.0f, 0.0f);
			if (total_presses % 3 == 1)
				red_state = glm::min(red_state + 0.5f, 1.0f);
			else if (total_presses % 3 == 2)
				green_state = glm::min(green_state + 0.5f, 1.0f);
		}
		blue_presses += 1;
	}
	total_presses += 1;
	//for remembering how to use to_string(): https://www.techiedelight.com/concatenate-integer-string-object-cpp/
	//for remembering how to use substr(): http://www.cplusplus.com/reference/string/string/substr/
	top_text = "RGB: " + std::to_string(red_state).substr(0, 3) + ", " + std::to_string(green_state).substr(0, 3) + ", " + std::to_string(blue_state).substr(0, 3);
	top_text += ("    Total Presses: " + std::to_string(total_presses) + ", Red Presses: " + std::to_string(red_presses) +
		", Green Presses: " + std::to_string(green_presses) + ", Blue Presses: " + std::to_string(blue_presses));
}