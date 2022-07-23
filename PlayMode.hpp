#include "Mode.hpp"

#include "Scene.hpp"
#include "Utils.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <deque>
#include <iostream>
#include <vector>

struct AssetMesh {
    AssetMesh() = default;

    std::unordered_map<std::string, Scene::Transform**> components;
};

struct PhysicalAssetMesh : AssetMesh {
    glm::vec3 pos, vel, accel;
    glm::vec3 rot, rotvel, rotaccel;

    PhysicalAssetMesh()
    {
        pos = glm::vec3(0, 0, 0);
        vel = glm::vec3(0, 0, 0);
        accel = glm::vec3(0, 0, -9.8); // gravity
        rot = glm::vec3(0, 0, 0);
        rotvel = glm::vec3(0, 0, 0);
        rotaccel = glm::vec3(0, 0, 0);
    }

    void update(const float dt)
    {
        // update positional kinematics
        vel += dt * accel;

        if (pos.z <= 0) {
            // downward velocity is 0 when on the ground
            vel.z = std::max(0.f, vel.z);
        }
        // std::cout << glm::to_string(vel) << std::endl;
        pos += dt * vel;
        pos.z = std::max(0.f, pos.z);

        // update rotational/angular kinematics
        rotvel += dt * rotaccel;
        rot += dt * rotvel;
        normalize(rot);
    }
};

struct FourWheeledVehicle : PhysicalAssetMesh {

    FourWheeledVehicle(const std::string& nameIn)
        : PhysicalAssetMesh()
    {
        name = nameIn;
    }

    bool bIsPlayer = false;
    Scene::Transform *all, *chassis, *wheel_FL, *wheel_FR, *wheel_BL, *wheel_BR;

    void initialize_components()
    {
        components[name] = &all;
        components["body"] = &chassis;
        components["wheel_frontLeft"] = &wheel_FL;
        components["wheel_frontRight"] = &wheel_FR;
        components["wheel_backLeft"] = &wheel_BL;
        components["wheel_backRight"] = &wheel_BR;
    }

    void initialize_from_scene(Scene& scene)
    {
        assert(scene.transforms.size() > 0);

        // add components to the global dictionary
        initialize_components();

        // get pointers to scene components for convenience:
        std::string suffix = "";

        // find the first (and should be only) instance of $name in scene
        bool found_name = false;
        for (auto& transform : scene.transforms) {
            if (!found_name) {
                if (transform.name == name) {
                    (*components[name]) = &transform;
                    found_name = true;
                }
            } else {
                if (transform.name.find("body") != std::string::npos) {
                    auto const pos = transform.name.find_last_of('.');
                    suffix = "." + transform.name.substr(pos + 1);
                    if (suffix == ".body") {
                        suffix = ""; // "." not found
                    }
                    break;
                }
            }
        }

        for (auto& s : components) {
            const std::string key = s.first;
            const std::string search = key + suffix;
            for (auto& transform : scene.transforms) {
                if (transform.name == search) { // contains key
                    (*s.second) = &transform;
                    // std::cout << "found " << transform.name << " to fit " << search << std::endl;
                    break;
                }
            }
            /// TODO: break early once all the components are found
            if (s.second == nullptr || (*s.second) == nullptr) {
                throw std::runtime_error("Unable to find " + name + "'s \"" + s.first + "\" in scene");
            }
        }

        if (all == nullptr)
            throw std::runtime_error("main object for \"" + name + "\" is null");
        pos = all->position;
        rot = glm::eulerAngles(all->rotation);
    }

    void update(const float dt)
    {
        woggle += 2 * dt;
        woggle -= std::floor(woggle);

        if (throttle > 0) {
            chassis->rotation = glm::angleAxis(glm::radians(std::sin(woggle * 2 * float(M_PI))), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        wheel_FL->rotation = glm::angleAxis(steer, glm::vec3(0, 0, 1));
        wheel_FR->rotation = glm::angleAxis(float(M_PI + steer), glm::vec3(0, 0, 1));

        // wheel rotation (stretch)
        // wheel_FL->rotation *= glm::angleAxis(-0.1f, glm::vec3(1.0f, 0.0f, 0.0f));
        // wheel_FR->rotation *= glm::angleAxis(-0.1f, glm::vec3(1.0f, 0.0f, 0.0f));
        // wheel_BL->rotation *= glm::angleAxis(-0.1f, glm::vec3(1.0f, 0.0f, 0.0f));
        // wheel_BR->rotation *= glm::angleAxis(-0.1f, glm::vec3(1.0f, 0.0f, 0.0f));

        if (pos.z <= 0) { // ground update
            // inspiration for this physics update 	was taken from this code:
            // https://github.com/winstxnhdw/KinematicBicycleModel

            // create 3D acceleration vector
            const float yaw = rot.z + (M_PI / 2);
            const glm::vec3 heading = glm::vec3(glm::cos(yaw), glm::sin(yaw), 0);
            accel = heading * (throttle_force * throttle - brake_force * brake) + glm::vec3(0, 0, accel.z);

            // compute forward speed
            glm::vec3 vel_2D = glm::vec3(vel.x, vel.y, 0);
            int velocity_sign = glm::sign(glm::dot(vel_2D, heading));
            float signed_speed = velocity_sign * glm::length(vel_2D);

            // compute friction
            float friction = signed_speed * (c_r + c_a * signed_speed);
            accel -= vel_2D * friction; // scale forward velocity by friction

            // ensure velocity in x/y is linked to heading
            vel.x = signed_speed * heading.x;
            vel.y = signed_speed * heading.y;

            // compute angular velocity (only along yaw)
            rotvel = (signed_speed * glm::tan(steer_force * steer) / wheel_diameter_m) * glm::vec3(0, 0, 1);
        } else { // in the air
            accel = glm::vec3(0, 0, -9.8);
        }

        // finally perform the physics update
        PhysicalAssetMesh::update(dt);
        all->position = pos;
        all->rotation = glm::quat(rot); // euler to Quat!
    }

    // how strong these effects get scaled
    float throttle_force = 10.f;
    float brake_force = 5.f; // brake or reverse?
    float steer_force = 1.f;

    // constants
    float wheel_diameter_m = 1.0f;
    float c_r = 0.02f; // coefficient of resistance
    float c_a = 0.25f; // drag coefficient
    float woggle = 0;

    // metadata
    std::string name = "";

    // control scheme inputs
    // throttle and brake are between 0..1, steer is between -PI..PI
    float throttle = 0.f, brake = 0.f, steer = 0.f;
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
    } left, right, down, up, jump;

    bool justJumped = false;

    // local copy of the game scene (so code can change it during gameplay):
    Scene scene;

    // all the vehicles in the scene
    std::vector<FourWheeledVehicle*> vehicle_map;
    FourWheeledVehicle* Player = nullptr;

    // camera:
    glm::vec2 move = glm::vec2(0, 0);
    float camera_arm_length = 15.f; // "distance" from camera to player
    glm::vec3 camera_offset = glm::vec3(0, -15, 15);
    float mouse_drag_speed_x = -10;
    float mouse_drag_speed_y = -10;
    float mouse_scroll_speed = 5;
    Scene::Camera* camera = nullptr;
};
