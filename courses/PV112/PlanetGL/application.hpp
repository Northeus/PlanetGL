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
const float clear_color[4] = {0.0, 0.0, 0.0, 1.0};
const float clear_depth[1] = {1.0};

class Application : public PV112Application {

    // ----------------------------------------------------------------------------
    // Variables
    // ----------------------------------------------------------------------------
    // Paths
    std::filesystem::path images_path = lecture_folder_path / "images";
    std::filesystem::path objects_path = lecture_folder_path / "objects";

    // Camera
    Camera camera;
    CameraUBO camera_ubo;
    GLuint camera_buffer = 0;

    // Light
    LightUBO light_ubo;
    GLuint light_buffer = 0;

    // Programs
    GLuint normal_program;
    GLuint postprocess_program;

    // Unit sphere
    Sphere unit_sphere;
    GLuint unit_sphere_buffer = 0;

    // Unit cube
    Cube unit_cube;

    // Buffers
    GLuint skybox_buffer;
    GLuint earth_buffer;
    GLuint sun_buffer;

    // Objects UBO
    ObjectUBO skybox_ubo;
    ObjectUBO earth_ubo;
    ObjectUBO sun_ubo;

    // Textures
    GLuint skybox_texture;
    GLuint earth_day_texture;
    GLuint earth_night_texture;
    GLuint sun_texture;

    // Frame buffer
    GLuint frame_buffer_name;
    GLuint render_texture;
    GLuint render_texture_vao;
    GLuint depth_buffer;

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
