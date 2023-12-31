// ################################################################################
// Common Framework for Computer Graphics Courses at FI MUNI.
//
// Copyright (c) 2021-2022 Visitlab (https://visitlab.fi.muni.cz)
// All rights reserved.
// ################################################################################

#pragma once

#include "camera.h"
#include "cube.hpp"
#include "geometry.hpp"
#include "pv112_application.hpp"
#include "sphere.hpp"
#include "teapot.hpp"
#include <memory>

// ----------------------------------------------------------------------------
// UNIFORM STRUCTS
// ----------------------------------------------------------------------------
struct CameraUBO {
    glm::mat4 projection;
    glm::mat4 view;
    glm::vec4 position;
};

struct LightUBO {
    glm::vec4 position;
    glm::vec4 ambient_color;
    glm::vec4 diffuse_color;
    glm::vec4 specular_color;
};

struct alignas(256) ObjectUBO {
    glm::mat4 model_matrix;  // [  0 -  64) bytes
    glm::vec4 ambient_color; // [ 64 -  80) bytes
    glm::vec4 diffuse_color; // [ 80 -  96) bytes

    // Contains shininess in .w element
    glm::vec4 specular_color; // [ 96 - 112) bytes
};

// Constants
const float blue_color[4] = {0.3, 0.3, 0.9, 1.0};
const float black_color[4] = {0.0, 0.0, 0.0, 1.0};
const float clear_depth[1] = {1.0};

class Application : public PV112Application {
    struct object {
        std::shared_ptr<Geometry> model;
        GLuint buffer;
        ObjectUBO ubo;
        GLuint texture; // Might be shared for some project, not needed here
        bool has_texture;
        bool ignore_light;

        ~object();
        void load_buffer();
    };

    struct frame_buffer {
        GLuint name;
        GLuint texture;

        ~frame_buffer();
        void load_buffer(int, int);
        void bind();
    };

    // ----------------------------------------------------------------------------
    // Variables
    // ----------------------------------------------------------------------------
    // Paths
    std::filesystem::path images_path = lecture_folder_path / "images";
    std::filesystem::path objects_path = lecture_folder_path / "objects";

    // Camera
    CameraUBO camera_room_ubo;
    GLuint camera_room_buffer = 0;
    glm::vec3 cam_room_front{0.0f, 0.0f, -1.0f};
    double yaw_room = -90.0f;
    double pitch_room = 0.0f;

    CameraUBO camera_space_ubo;
    GLuint camera_space_buffer = 0;
    glm::vec3 cam_space_front{0.0f, 0.0f, -1.0f};
    double yaw_space = -90.0;
    double pitch_space = 0.0;

    bool first_move = true;
    double lastX, lastY;

    bool pressed = false;
    float sensitivity = 0.3f;
    double speed = 0.02f;
    bool w_hold = false;
    bool s_hold = false;

    // Framebuffers
    frame_buffer space_bf;
    frame_buffer screen_bf;

    // Programs
    GLuint normal_program;
    GLuint postprocess_program;
    GLuint screen_program;

    // Helper objects
    object sphere;

    // Universe scene
    LightUBO light_ubo;
    GLuint light_buffer = 0;

    // Space scene
    object earth;
    object sun_space;
    object rocket;

    // Atmosphere parameters
    bool show_menu = false;
    int number_of_measurements = 10;
    int number_of_optical_depths = 10;
    float density_falloff = 4.3f;
    glm::vec3 wave_lengths = glm::vec3(700, 530, 440);
    float scattering_strength = 8.0f;

    // Room scene
    LightUBO light_room_ubo[2];
    GLuint light_room_buffer = 0;

    object room;
    object nature;
    std::array<object, 7> chickens;
    object airplane;
    object sun_room;
    object screen;

    void mko(object& obj, const std::string&, glm::mat4,
        std::shared_ptr<Geometry> = nullptr, bool = true, bool = false);
    void dro(object& obj);

    void mkf(frame_buffer&);

    bool is_space_scene = true;

    // Renders
    void render_universe();
    void render_scene();

    void print_hello();

  public:
    // ----------------------------------------------------------------------------
    // Constructors & Destructors
    // ----------------------------------------------------------------------------
    /**
     * Constructs a new @link Application with a custom width and height.
     *
     * @param 	initial_width 	The initial width of the window.
     * @param 	initial_height	The initial height of the window.
     * @param 	arguments	  	The command line arguments used to obtain the application directory.
     */
    Application(int initial_width, int initial_height, std::vector<std::string> arguments = {});

    /** Destroys the {@link Application} and releases the allocated resources. */
    ~Application() override;

    // ----------------------------------------------------------------------------
    // Methods
    // ----------------------------------------------------------------------------

    /** @copydoc PV112Application::compile_shaders */
    void compile_shaders() override;

    /** @copydoc PV112Application::delete_shaders */
    void delete_shaders() override;

    /** @copydoc PV112Application::update */
    void update(float delta) override;

    /** @copydoc PV112Application::render */
    void render() override;

    /** @copydoc PV112Application::render_ui */
    void render_ui() override;

    // ----------------------------------------------------------------------------
    // Input Events
    // ----------------------------------------------------------------------------

    /** @copydoc PV112Application::on_resize */
    void on_resize(int width, int height) override;

    /** @copydoc PV112Application::on_mouse_move */
    void on_mouse_move(double x, double y) override;

    /** @copydoc PV112Application::on_mouse_button */
    void on_mouse_button(int button, int action, int mods) override;

    /** @copydoc PV112Application::on_key_pressed */
    void on_key_pressed(int key, int scancode, int action, int mods) override;
};
