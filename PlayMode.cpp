#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Load.hpp"
#include "Mesh.hpp"
#include "data_path.hpp"
#include "gl_errors.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <algorithm>
#include <random>

GLuint program = 0;
Load<MeshBuffer> load_meshes(LoadTagDefault, []() -> MeshBuffer const* {
    MeshBuffer const* ret = new MeshBuffer(data_path("world.pnct"));
    program = ret->make_vao_for_program(lit_color_texture_program->program);
    return ret;
});

Load<Scene> load_scene(LoadTagDefault, []() -> Scene const* {
    return new Scene(data_path("world.scene"), [&](Scene& scene, Scene::Transform* transform, std::string const& mesh_name) {
        Mesh const& mesh = load_meshes->lookup(mesh_name);

        scene.drawables.emplace_back(transform);
        Scene::Drawable& drawable = scene.drawables.back();

        drawable.pipeline = lit_color_texture_program_pipeline;

        drawable.pipeline.vao = program;
        drawable.pipeline.type = mesh.type;
        drawable.pipeline.start = mesh.start;
        drawable.pipeline.count = mesh.count;
    });
});

PlayMode::PlayMode()
    : scene(*load_scene)
{

    std::vector<std::string> vehicle_names = {
        "ambulance",
        "delivery",
        "deliveryFlat",
        "firetruck",
        "garbageTruck",
        "hatchbackSports",
        "police",
        "race",
        "sedan",
        "sedanSports",
        "suv",
        "suvLuxury",
        "taxi",
        "tractor",
        "tractorPolice",
        "tractorShovel",
        "truck",
        "truckFlat",
        "van"
    };

    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(vehicle_names), std::end(vehicle_names), rng);

    for (const std::string& name : vehicle_names) {
        FourWheeledVehicle* FWV = new FourWheeledVehicle(name);
        FWV->initialize_from_scene(scene);
        vehicle_map.push_back(FWV);
        // the first vehicle will be the player
    }

    Player = vehicle_map[0];
    std::cout << "Determined player to be \"" << Player->name << "\"" << std::endl;
    Player->bIsPlayer = true;

    // get pointer to camera for convenience:
    if (scene.cameras.size() != 1)
        throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
    camera = &scene.cameras.front();
}

PlayMode::~PlayMode()
{
}

bool PlayMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size)
{

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
            jump.downs += 1;
            jump.pressed = true;
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
            jump.pressed = false;
            return true;
        }
    } else if (evt.type == SDL_MOUSEBUTTONDOWN) {
        if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
            SDL_SetRelativeMouseMode(SDL_TRUE);
            return true;
        }
    } else if (evt.type == SDL_MOUSEMOTION) {
        if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
            move = glm::vec2(
                evt.motion.xrel / float(window_size.y),
                -evt.motion.yrel / float(window_size.y));
            return true;
        }
    }

    return false;
}

void PlayMode::update(float elapsed)
{

    // update all the vehicles
    for (FourWheeledVehicle* FWV : vehicle_map) {
        FWV->update(elapsed);
    }

    {
        // combine inputs into a move:
        if (left.pressed || right.pressed) {
            const float wheel_turn_rate = Player->pos.z > 0 ? 2.f : 0.5f; // how many radians per second are turned
            if (left.pressed && !right.pressed)
                Player->steer = std::min(float(M_PI / 4), Player->steer + elapsed * wheel_turn_rate);
            if (!left.pressed && right.pressed)
                Player->steer = std::max(float(-M_PI / 4), Player->steer - elapsed * wheel_turn_rate);
        } else {
            // force feedback return steering wheel to 0
            Player->steer += elapsed * 2.f * (0 - Player->steer);
        }
        if (jump.pressed) {
            if (!justJumped && Player->pos.z == 0) {
                // give some initial velocity
                Player->vel += glm::vec3(0, 0, 15);
                justJumped = true;
            }
        } else {
            justJumped = false;
        }

        // std::cout << Player->steer << std::endl;

        if (up.pressed || down.pressed) {
            if (down.pressed && !up.pressed) {
                Player->throttle = 0;
                Player->brake = 1;
            }
            if (!down.pressed && up.pressed) {
                Player->throttle = 1;
                Player->brake = 0;
            }
        } else {
            Player->throttle = 0;
            Player->brake = 0;
        }
        /// TODO: rotate camera?
        camera->transform->position = Player->pos + camera_offset;
    }

    // move camera:
    {
        // camera->transform->position += glm::vec3(motion.x, motion.y, 0);

        glm::mat4x3 frame = camera->transform->make_local_to_parent();
        glm::vec3 right = frame[0];
        glm::vec3 up = frame[1];
        // glm::vec3 forward = -frame[2];

        camera_offset += mouse_drag_speed_x * move.x * right + mouse_drag_speed_y * move.y * up; // + mouse_scroll_speed * move.z * forward;

        // camera_offset.y = std::min(-1.f, std::max(camera_offset.y, -camera_arm_length)); // forward (negative bc looking behind vehicle)
        camera_offset.z = std::min(camera_arm_length, std::max(camera_offset.z, 1.f)); // vertical
        // normalize camera so its always camera_arm_length away
        camera_offset /= glm::length(camera_offset);
        camera_offset *= camera_arm_length;

        glm::vec3 dir = glm::normalize(Player->all->position - camera->transform->position);
        camera->transform->rotation = glm::quatLookAt(dir, glm::vec3(0, 0, 1));
    }

    // reset button press counters:
    left.downs = 0;
    right.downs = 0;
    up.downs = 0;
    down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const& drawable_size)
{
    // update camera aspect ratio for drawable:
    camera->aspect = float(drawable_size.x) / float(drawable_size.y);

    // set up light type and position for lit_color_texture_program:
    //  TODO: consider using the Light(s) in the scene to do this
    glUseProgram(lit_color_texture_program->program);
    glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
    glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
    glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
    glUseProgram(0);

    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // this is the default depth comparison function, but FYI you can change it.

    GL_ERRORS(); // print any errors produced by this setup code

    scene.draw(*camera);

    { // use DrawLines to overlay some text:
        glDisable(GL_DEPTH_TEST);
        float aspect = float(drawable_size.x) / float(drawable_size.y);
        DrawLines lines(glm::mat4(
            1.0f / aspect, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f));

        constexpr float H = 0.09f;
        lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
            glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
            glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
            glm::u8vec4(0x00, 0x00, 0x00, 0x00));
        float ofs = 2.0f / drawable_size.y;
        lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
            glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
            glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
            glm::u8vec4(0xff, 0xff, 0xff, 0x00));
    }
}
