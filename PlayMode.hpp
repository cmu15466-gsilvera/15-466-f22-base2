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

    FourWheeledVehicle()
        : PhysicalAssetMesh()
    {
    }

    Scene::Transform *all, *chassis, *wheel_FL, *wheel_FR, *wheel_BL, *wheel_BR;

    void initialize_components(const std::string& name)
    {
        components[name] = &all;
        components["body"] = &chassis;
        components["wheel_frontLeft"] = &wheel_FL;
        components["wheel_frontRight"] = &wheel_FR;
        components["wheel_backLeft"] = &wheel_BL;
        components["wheel_backRight"] = &wheel_BR;
    }

    void initialize_from_scene(Scene& scene, const std::string& name)
    {
        assert(scene.transforms.size() > 0);

        // add components to the global dictionary
        initialize_components(name);

        // get pointers to scene components for convenience:
        for (auto& s : components) {
            const std::string key = s.first;

            for (auto& transform : scene.transforms) {
                if (transform.name == key) {
                    (*s.second) = &transform;
                }
            }
            if ((*s.second) == nullptr)
                throw std::runtime_error("Unable to find \"" + key + "\" in scene!");
        }

        assert(all != nullptr);
        all->position = glm::vec3(0, 0, 0);
        all->rotation = glm::identity<glm::quat>();
        all->scale = glm::vec3(1, 1, 1);
    }

    void update(const float dt)
    {
        if (pos.z <= 0) { // ground update
            // inspiration for this physics update was taken from this code:
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

    FourWheeledVehicle ambulance;
    float woggle = 0;

    // camera:
    // float camera_orbit_yaw = 0.f;
    // float camera_orbit_pitch = 0.f;
    glm::vec2 move = glm::vec2(0, 0);
	float camera_arm_length = 15.f; // "distance" from camera to player
    glm::vec3 camera_offset = glm::vec3(0, -15, 15);
    float mouse_drag_speed_x = -10;
    float mouse_drag_speed_y = -10;
    float mouse_scroll_speed = 5;
    Scene::Camera* camera = nullptr;
};
