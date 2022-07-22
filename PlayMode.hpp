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

struct PhysicalAssetMesh : AssetMesh {
    glm::vec3 pos, vel, accel;
    glm::vec3 rot, rotvel, rotaccel;

    PhysicalAssetMesh()
    {
        pos = glm::vec3(0, 0, 0);
        vel = glm::vec3(0, 0, 0);
        accel = glm::vec3(0, 0, 0);
        rot = glm::vec3(0, 0, 0);
        rotvel = glm::vec3(0, 0, 0);
        rotaccel = glm::vec3(0, 0, 0);
    }

    void update(const float dt)
    {
        // update positional kinematics
        vel += dt * accel;
        pos += dt * vel;

        // update rotational/angular kinematics
        rotvel += dt * rotaccel;
        rot += dt * rotvel;
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

    float normalize_angle_rad(float angle) const
    {
        float delta = 0.f;
        if (angle > M_PI) {
            delta = -2.f * M_PI;
        } else if (yaw < -M_PI) {
            delta = 2.f * M_PI;
        }
        return angle + delta;
    }

    void update(const float dt)
    {
        // inspiration for this physics update was taken from this code:
        // https://github.com/winstxnhdw/KinematicBicycleModel

        // create 3D velocity vector

        glm::vec3 heading = glm::vec3(glm::cos(yaw), glm::sin(yaw), 0);
        int velocity_sign = glm::sign(glm::dot(vel, heading));

        // compute forward speed
        float speed = velocity_sign * glm::length(vel);
        speed = speed + dt * (10.f * throttle - 5.f * brake);
        float friction = speed * (c_r + c_a * speed);
        speed -= dt * glm::sign(speed) * friction; // apply friction

        vel = speed * heading;

        // Compute the angular velocity
        float angular_vel = speed * glm::tan(steer_angle) / wheel_diameter_m;

        // compute the global yaw direction
        yaw = normalize_angle_rad(yaw + angular_vel * dt);

        // finally perform the physics update
        PhysicalAssetMesh::update(dt);
        all->position = pos;
        all->rotation = glm::angleAxis(float(yaw - yaw0), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    float wheel_diameter_m = 1.0f;
    float c_r = 0.1f; // coefficient of resistance
    float c_a = 0.9f; // coefficient of aerodynamics

    float speed = 0; // m/s
    float steer_angle = 0.f; // in radians

    float yaw = M_PI / 2; // global heading (radians)
    float yaw0 = M_PI / 2; // initial heading (radians)

    float throttle = 0, brake = 0;
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
    float woggle = 0;

    // camera:
    Scene::Camera* camera = nullptr;
};
