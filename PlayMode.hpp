#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <deque>
#include <iostream>
#include <vector>

struct AssetMesh {
    AssetMesh() = default;

    std::unordered_map<std::string, Scene::Transform**> components;
};

struct FourWheeledVehicle : AssetMesh {

    Scene::Transform *chassis, *wheel_FL, *wheel_FR, *wheel_BL, *wheel_BR;

    void initialize_components()
    {
        components["body"] = &chassis;
        components["wheel_frontLeft"] = &wheel_FL;
        components["wheel_frontRight"] = &wheel_FR;
        components["wheel_backLeft"] = &wheel_BL;
        components["wheel_backRight"] = &wheel_BR;
    }

    void initialize_with_scene(Scene& scene)
    {
        // add components to the global dictionary
        initialize_components();

        // get pointers to scene components for convenience:
        for (auto& s : components) {
            const std::string key = s.first;

            for (auto& transform : scene.transforms) {
                if (transform.name == key) {
                    (*s.second) = &transform;
                }
            }
            if (s.second == nullptr)
                throw std::runtime_error("Unable to find \"" + key + "\" in scene!");
        }
    }

    // glm::quat body_rotation;
    // glm::quat front_wheel_rotation;
    // glm::quat back_wheel_rotation;
    float speed;
};

struct PlayMode : Mode {
    PlayMode();
    virtual ~PlayMode();

    // functions called by main loop:
    virtual bool handle_event(SDL_Event const&, glm::uvec2 const& window_size) override;
    virtual void update(float elapsed) override;
    virtual void draw(glm::uvec2 const& drawable_size) override;

    //----- game state -----

    // input tracking:
    struct Button {
        uint8_t downs = 0;
        uint8_t pressed = 0;
    } left, right, down, up;

    // local copy of the game scene (so code can change it during gameplay):
    Scene scene;

    FourWheeledVehicle ambulance;

    // camera:
    Scene::Camera* camera = nullptr;
};
