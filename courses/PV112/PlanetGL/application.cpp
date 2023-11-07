#include "application.hpp"

#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using std::make_shared;

GLuint load_texture_2d(const std::filesystem::path filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename.generic_string().data(), &width, &height, &channels, 4);

    GLuint texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);

    glTextureStorage2D(texture, std::log2(width), GL_RGBA8, width, height);

    glTextureSubImage2D(texture,
                        0,                         //
                        0, 0,                      //
                        width, height,             //
                        GL_RGBA, GL_UNSIGNED_BYTE, //
                        data);

    stbi_image_free(data);

    glGenerateTextureMipmap(texture);

    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texture;
}

Application::Application(int initial_width, int initial_height, std::vector<std::string> arguments)
    : PV112Application(initial_width, initial_height, arguments) {
    this->width = initial_width;
    this->height = initial_height;

    images_path = lecture_folder_path / "images";
    objects_path = lecture_folder_path / "objects";

    // --------------------------------------------------------------------------
    //  Load/Create Objects
    // --------------------------------------------------------------------------
    geometries.push_back(make_shared<Sphere>());
    // You can use from_file function to load a Geometry from .obj file
    geometries.push_back(make_shared<Geometry>(Geometry::from_file(objects_path / "bunny.obj")));

    sphere = geometries[0];
    bunny = geometries[1];

    marble_texture = load_texture_2d(images_path / "bunny.jpg");

    // --------------------------------------------------------------------------
    // Initialize UBO Data
    // --------------------------------------------------------------------------
    camera_ubo.position = glm::vec4(camera.get_eye_position(), 1.0f);
    camera_ubo.projection = glm::perspective(glm::radians(45.0f), float(width) / float(height), 0.01f, 1000.0f);
    camera_ubo.view = glm::lookAt(camera.get_eye_position(), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    light_ubo.position = glm::vec4(0.0f, 3.0f, 2.0f, 1.0f);
    light_ubo.ambient_color = glm::vec4(1.0f);
    light_ubo.diffuse_color = glm::vec4(2.0f, 2.0f, 1.0f, 1.0f);
    light_ubo.specular_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    objects_ubos.push_back({.model_matrix = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)), glm::vec3(light_ubo.position)),
                            .ambient_color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
                            .diffuse_color = glm::vec4(0.0),
                            .specular_color = glm::vec4(0.0f)});
    objects_ubos.push_back({.model_matrix = glm::mat4(1.0f),
                            .ambient_color = glm::vec4(0.0f),
                            .diffuse_color = glm::vec4(1.0f),
                            .specular_color = glm::vec4(0.0f)});

    // --------------------------------------------------------------------------
    // Create Buffers
    // --------------------------------------------------------------------------
    glCreateBuffers(1, &camera_buffer);
    glNamedBufferStorage(camera_buffer, sizeof(CameraUBO), &camera_ubo, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &light_buffer);
    glNamedBufferStorage(light_buffer, sizeof(LightUBO), &light_ubo, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &objects_buffer);
    glNamedBufferStorage(objects_buffer, sizeof(ObjectUBO) * objects_ubos.size(), objects_ubos.data(), GL_DYNAMIC_STORAGE_BIT);

    compile_shaders();
}

Application::~Application() {
    delete_shaders();

    glDeleteBuffers(1, &camera_buffer);
    glDeleteBuffers(1, &light_buffer);
    glDeleteBuffers(1, &objects_buffer);
}

// ----------------------------------------------------------------------------
// Methods
// ----------------------------------------------------------------------------

void Application::delete_shaders() {}

void Application::compile_shaders() {
    delete_shaders();
    main_program = create_program(lecture_shaders_path / "main.vert", lecture_shaders_path / "main.frag");
}

void Application::update(float delta) {}

void Application::render() {
    // --------------------------------------------------------------------------
    // Update UBOs
    // --------------------------------------------------------------------------
    // Camera
    camera_ubo.position = glm::vec4(camera.get_eye_position(), 1.0f);
    camera_ubo.projection = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.01f, 1000.0f);
    camera_ubo.view = glm::lookAt(camera.get_eye_position(), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glNamedBufferSubData(camera_buffer, 0, sizeof(CameraUBO), &camera_ubo);

    // --------------------------------------------------------------------------
    // Draw scene
    // --------------------------------------------------------------------------
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    // Clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Configure fixed function pipeline
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Draw objects
    glUseProgram(main_program);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, camera_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, light_buffer);
    glBindBufferRange(GL_UNIFORM_BUFFER, 2, objects_buffer, 0 * 256, sizeof(ObjectUBO));
    
    glUniform1i(glGetUniformLocation(main_program, "has_texture"), false);
    sphere->draw();

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, camera_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, light_buffer);
    glBindBufferRange(GL_UNIFORM_BUFFER, 2, objects_buffer, 1 * 256, sizeof(ObjectUBO));

    glUniform1i(glGetUniformLocation(main_program, "has_texture"), true);
    glBindTextureUnit(3, marble_texture);
    bunny->draw();
}

void Application::render_ui() { const float unit = ImGui::GetFontSize(); }

// ----------------------------------------------------------------------------
// Input Events
// ----------------------------------------------------------------------------

void Application::on_resize(int width, int height) {
    // Calls the default implementation to set the class variables.
    PV112Application::on_resize(width, height);
}

void Application::on_mouse_move(double x, double y) { camera.on_mouse_move(x, y); }
void Application::on_mouse_button(int button, int action, int mods) { camera.on_mouse_button(button, action, mods); }
void Application::on_key_pressed(int key, int scancode, int action, int mods) {
    // Calls default implementation that invokes compile_shaders when 'R key is hit.
    PV112Application::on_key_pressed(key, scancode, action, mods);
}