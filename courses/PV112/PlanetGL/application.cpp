#include "application.hpp"
#include "cube.hpp"
#include "geometry.hpp"
#include "sphere.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <memory>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

GLuint load_texture_2d(const std::filesystem::path filename) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename.generic_string().data(), &width, &height, &channels, 4);

    GLuint texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);

    glTextureStorage2D(texture, std::log2(std::min(width, height)), GL_RGBA8, width, height);

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

Application::object::~object()
{
    glDeleteBuffers(1, &buffer);

    if (has_texture)
        glDeleteTextures(1, &texture);
}

void Application::mko(
    Application::object& obj, const std::string& name,
    glm::mat4 model_matrix, std::shared_ptr<Geometry> model_ptr,
    bool has_texture, bool ignore_light )
{
    obj.has_texture = has_texture;
    obj.ignore_light = ignore_light;

    if (has_texture)
    {
        obj.texture = load_texture_2d(images_path / ( name + ".jpg"));
    }

    obj.model = model_ptr
        ? model_ptr
        : std::make_shared<Geometry>(Geometry::from_file(objects_path / ( name + ".obj")));

    obj.ubo.model_matrix = model_matrix;
    obj.ubo.ambient_color = glm::vec4(0.5f);
    obj.ubo.diffuse_color = glm::vec4(1.0f);
    obj.ubo.specular_color = glm::vec4(0.0f);

    obj.load_buffer();
}

void Application::object::load_buffer()
{
    glCreateBuffers(1, &buffer);
    glNamedBufferStorage(
        buffer,
        sizeof(ObjectUBO),
        &ubo,
        0);
}

void Application::mkf(Application::frame_buffer& f)
{
    glCreateFramebuffers(1, &f.name);
    f.load_buffer(width, height);
}

void Application::print_hello()
{
    // In case of windows
    std::cout << "Controls: W - go forward\r\n"
              << "          S - go backward\r\n"
              << "          A - increase speed\r\n"
              << "          D - decrease speed\r\n"
              << "          Q - show menu in space\r\n"
              << "          E - switch dimension\r\n"
              << "          Hold right button to look around\r\n\n"
              << "Checklist:\r\n"
              << " - 4 unique complex objects (nature, room, chicken, airplane)\r\n"
              << " - 12 objects in room scene (7 chickens, airplane, room, surface, sun, screen)\r\n"
              << " - 5 textures in room scene (screen, airplane, room, surface, chicken)\r\n"
              << " - 2 lights in room scene (sun, lamp)\r\n"
              << " - Earth rotates in space\r\n"
              << " - In room scene everything is lit\r\n"
              << " - Atmosphere (scattering, fog, raytracing, physics)\r\n"
              << " - Projection texture (space to screen)\r\n"
              << " - Audio" << std::endl;
}

Application::Application(int initial_width, int initial_height, std::vector<std::string> arguments)
    : PV112Application(initial_width, initial_height, arguments) {
    print_hello();

    this->width = initial_width;
    this->height = initial_height;

    // --------------------------------------------------------------------------
    // Initialize UBO Data
    // --------------------------------------------------------------------------
    for ( CameraUBO* ubo : { &camera_room_ubo, &camera_space_ubo } ) {
        ubo->projection = glm::perspective(
            glm::radians(45.0f),
            float(width) / float(height),
            0.01f, 1000.0f);
        ubo->position = glm::vec4(camera.get_eye_position(), 1.0f);
        ubo->view = glm::lookAt(
            glm::vec3(ubo->position),
            glm::vec3(0.0f, 0.0f, -1.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));
    }

    light_ubo.position = glm::vec4(10.0f, 10.0f, -10.0f, 0.0f);
    light_ubo.ambient_color = glm::vec4(1.0f);
    light_ubo.diffuse_color = glm::vec4(1.0f);
    light_ubo.specular_color = glm::vec4(1.0f);

    light_room_ubo[0].position = glm::vec4(25.0f, 40.0f, -25.0f, 0.0f);
    light_room_ubo[0].ambient_color = glm::vec4(0.8f, 0.6f, 0.2f, 1.0f);
    light_room_ubo[0].diffuse_color = glm::vec4(1.0f);
    light_room_ubo[0].specular_color = glm::vec4(1.0f);

    light_room_ubo[1].position = glm::vec4(-16.6f, -0.6f, 19.9f, 1.0f);
    light_room_ubo[1].ambient_color = glm::vec4(glm::vec3(0.2f), 1.0f);
    light_room_ubo[1].diffuse_color = glm::vec4(0.4f);
    light_room_ubo[1].specular_color = glm::vec4(0.6f);

    // --------------------------------------------------------------------------
    //  Load/Create Objects
    // --------------------------------------------------------------------------
    mko(sphere, "", glm::mat4(1.0f), std::make_shared<Geometry>(Sphere()), false, true);

    auto earth_model_matrix = glm::translate(glm::vec3(0.0f, 0.0f, 1.0f));
    mko(earth, "earth", earth_model_matrix, sphere.model, true, false);
    earth.ubo.ambient_color = glm::vec4(0.0f);
    earth.ubo.diffuse_color = glm::vec4(0.3f);
    earth.ubo.specular_color = glm::vec4(0.0f);
    earth.load_buffer();

    auto sun_model_matrix = glm::translate(
        glm::scale(glm::vec3{5.0f, 5.0f, 5.0f}),
        glm::vec3(light_ubo.position));
    mko(sun_space, "sun", sun_model_matrix, sphere.model, true, false);
    sun_space.ubo.ambient_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    sun_space.ubo.diffuse_color = glm::vec4(0.0f);
    sun_space.ubo.specular_color = glm::vec4(0.0f);
    sun_space.load_buffer();

    mko(rocket, "rocket", glm::mat4(1.0f));

    mko(nature, "nature", glm::scale(glm::vec3(50.0f)));
    mko(room, "room", glm::translate(glm::scale(glm::vec3(10.0f)),
                                     glm::vec3(-1.6f, 0.05f, 1.6f)));
    mko(airplane, "airplane", glm::translate(glm::scale(glm::vec3(10.0f)),
                                             glm::vec3(0.0f, 2.0f, 0.0f)));

    mko(chickens[ 0 ], "chicken", glm::translate(glm::vec3(2.f, -3.3f, 1.f)));

    for (auto [ i, x, y ] : std::vector<std::tuple<int, double, double>>{
                {1,  2., -2.},
                {2, -2.,  2.},
                {3,  0.,  0.},
                {4, -4., -4.},
                {5, -2., -4.},
                {6, -4., -3.},
            })
    {
        mko(
            chickens[ i ], "chicken",
            glm::translate(glm::vec3(-0.f + x, -3.3f, -3.f + y)),
            chickens[0].model);
    }

    mko(sun_room, "", glm::translate(glm::scale(glm::vec3(2.0f)),
                                     glm::vec3(light_ubo.position)),
        std::make_shared<Geometry>(Sphere()), false, true);;
    sun_room.ubo.ambient_color = glm::vec4(0.9f, 0.7f, 0.3f, 1.0f);
    sun_room.load_buffer();

    std::vector<float> positions = {
        -12.6958f,      -0.1198f,       19.8613f,
        -14.0852f,      -0.1194f,       19.8625f,
        -14.0843f,      0.6850f,        19.8638f,
        -12.6958f,      0.6860f,        19.8631f
    };

    std::vector<float> texture_coords = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    std::vector<uint32_t> indices = {
        0u, 1u, 2u,
        0u, 2u, 3u
    };

    mko(screen, "sun", glm::mat4(1.0f), std::make_shared<Geometry>(
        Geometry(GL_TRIANGLES, positions, indices, {}, {}, texture_coords)), true, true);
    screen.ubo.ambient_color = glm::vec4(1.0f);
    screen.load_buffer();

    // --------------------------------------------------------------------------
    // Create Buffers
    // --------------------------------------------------------------------------
    glCreateBuffers(1, &camera_room_buffer);
    glNamedBufferStorage(camera_room_buffer, sizeof(CameraUBO), &camera_room_ubo, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &camera_space_buffer);
    glNamedBufferStorage(camera_space_buffer, sizeof(CameraUBO), &camera_room_ubo, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &light_buffer);
    glNamedBufferStorage(light_buffer, sizeof(LightUBO), &light_ubo, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &light_room_buffer);
    glNamedBufferStorage(light_room_buffer, 2 * sizeof(LightUBO),
        &light_room_ubo, GL_DYNAMIC_STORAGE_BIT);

    mkf(space_bf);
    mkf(screen_bf);

    compile_shaders();
}

Application::~Application() {
    delete_shaders();

    glDeleteBuffers(1, &light_buffer);
    glDeleteBuffers(1, &camera_space_buffer);
    glDeleteBuffers(1, &camera_room_buffer);
    glDeleteBuffers(1, &light_buffer);
    glDeleteBuffers(1, &light_room_buffer);
}

// ----------------------------------------------------------------------------
// Methods
// ----------------------------------------------------------------------------

void Application::delete_shaders() {
    glDeleteProgram(normal_program);
    glDeleteProgram(postprocess_program);
    glDeleteProgram(screen_program);
}

void Application::compile_shaders() {
    delete_shaders();
    normal_program = create_program(
        lecture_shaders_path / "normal.vert",
        lecture_shaders_path / "normal.frag");

    postprocess_program = create_program(
        lecture_shaders_path / "postprocess.vert",
        lecture_shaders_path / "postprocess.frag");

    screen_program = create_program(
        lecture_shaders_path / "postprocess.vert",
        lecture_shaders_path / "screen.frag");
}

void Application::update(float delta) {
    auto& cam = is_space_scene ? camera_space_ubo : camera_room_ubo;
    auto& cam_front = is_space_scene ? cam_space_front : cam_room_front;

    if (w_hold)
        cam.position += glm::vec4(cam_front, 0.0f) * speed * delta;
    if (s_hold)
        cam.position -= glm::vec4(cam_front, 0.0f) * speed * delta;

    earth.ubo.model_matrix = glm::rotate(
        earth.ubo.model_matrix,
        glm::radians(delta * 0.02f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    earth.load_buffer();
}

void Application::frame_buffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, name);
    glClearNamedFramebufferfv(name, GL_COLOR, 0, black_color);
}

void Application::render() {
    const float* clear_color = black_color;

    // --------------------------------------------------------------------------
    // Update UBOs
    // --------------------------------------------------------------------------
    auto& camera_ubo = is_space_scene ? camera_space_ubo : camera_room_ubo;
    auto& camera_buffer = is_space_scene ? camera_space_buffer : camera_room_buffer;
    auto& cam_front = is_space_scene ? cam_space_front : cam_room_front;

    camera_ubo.view = glm::lookAt(
        glm::vec3(camera_ubo.position),
        glm::vec3(camera_ubo.position) + cam_front,
        glm::vec3(0.0f, 1.0f, 0.0f));

    glNamedBufferSubData(camera_buffer, 0, sizeof(CameraUBO), &camera_ubo);

    // --------------------------------------------------------------------------
    // Render scene
    // --------------------------------------------------------------------------
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    // Render raw space
    space_bf.bind();

    glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    render_universe();

    screen_bf.bind();

    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(postprocess_program);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, camera_space_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, light_buffer);
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(camera_space_ubo.projection));
    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(camera_space_ubo.view));

    glUniform1i(3, number_of_measurements);
    glUniform1i(4, number_of_optical_depths);
    glUniform1f(5, density_falloff);
    glUniform3fv(6, 1, glm::value_ptr(wave_lengths));
    glUniform1f(7, scattering_strength);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, space_bf.texture);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (is_space_scene) {
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(screen_program);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, screen_bf.texture);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    else
    {
        clear_color = blue_color;

        glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        render_scene();
    }
}

void Application::render_universe()
{
    glUseProgram(normal_program);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, camera_space_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, light_buffer);

    glUniform1i(glGetUniformLocation(normal_program, "light_count"), 1);

    dro(sun_space);
    dro(earth);
    //dro(rocket);
}

void Application::render_scene()
{
    glUseProgram(normal_program);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, camera_room_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, light_room_buffer);

    glUniform1i(glGetUniformLocation(normal_program, "light_count"), 2);

    dro(room);
    dro(nature);
    dro(airplane);
    dro(sun_room);

    for (auto& o : chickens)
    {
        dro(o);
    }

    // A bit nasty trick so i don't have to make interface more complicated (destructors)
    std::swap(screen.texture, screen_bf.texture);
    dro(screen);
    std::swap(screen.texture, screen_bf.texture);
}

void Application::dro(Application::object& o)
{
    glUniform1i(glGetUniformLocation(normal_program, "has_texture"), o.has_texture);
    glUniform1i(glGetUniformLocation(normal_program, "ignore_light"), o.ignore_light);

    if (o.has_texture)
        glBindTextureUnit(3, o.texture);

    glBindBufferRange(GL_UNIFORM_BUFFER, 2, o.buffer, 0, sizeof(ObjectUBO));

    o.model->draw();
}

void Application::render_ui() {
    if (!is_space_scene) {
        const float unit = ImGui::GetFontSize();

        ImGui::Begin("Parameters", nullptr, ImGuiWindowFlags_NoDecoration);
        ImGui::SetWindowSize(ImVec2(14 * unit, 2 * unit));
        ImGui::SetWindowPos(ImVec2(1 * unit, 1 * unit));

        ImGui::Text("Press E to change dimension");

        ImGui::End();

        return;
    }

    const float unit = ImGui::GetFontSize();

    if (show_menu) {
        ImGui::Begin("Parameters", nullptr, ImGuiWindowFlags_NoDecoration);
        ImGui::SetWindowSize(ImVec2(32 * unit, 9 * unit));
        ImGui::SetWindowPos(ImVec2(1 * unit, 1 * unit));

        ImGui::SliderInt("Measurements", &number_of_measurements, 1, 20);
        ImGui::SliderInt("Optical depths", &number_of_optical_depths, 1, 20);
        ImGui::SliderFloat("Density falloff", &density_falloff, 0.0f, 20.0f);
        ImGui::SliderFloat("Scattering strength", &scattering_strength, 0.0f, 40.0f);
        ImGui::SliderFloat3("Wavelengths", glm::value_ptr(wave_lengths), 0.0f, 1000.0f);

        ImGui::End();
    } else {
        ImGui::Begin("Parameters", nullptr, ImGuiWindowFlags_NoDecoration);
        ImGui::SetWindowSize(ImVec2(11 * unit, 2 * unit));
        ImGui::SetWindowPos(ImVec2(1 * unit, 1 * unit));

        ImGui::Text("Press Q to show menu");

        ImGui::End();
    }
}

// ----------------------------------------------------------------------------
// Input Events
// ----------------------------------------------------------------------------
Application::frame_buffer::~frame_buffer()
{
    glDeleteTextures(1, &texture);
    glDeleteFramebuffers(1, &name);
}

void Application::frame_buffer::load_buffer(int w, int h)
{
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureStorage2D(texture, 1, GL_RGBA32F, w, h);

    const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
    glNamedFramebufferDrawBuffers(name, 1, draw_buffers);

    glNamedFramebufferTexture(name, GL_COLOR_ATTACHMENT0, texture, 0);
}

void Application::on_resize(int width, int height) {
    space_bf.load_buffer(width, height);
    screen_bf.load_buffer(width, height);

    // Calls the default implementation to set the class variables.
    PV112Application::on_resize(width, height);
}

// learnopengl
void Application::on_mouse_move(double x, double y) {
    if (first_move) {
        lastX = x;
        lastY = y;
        first_move = false;
    }

    double offsetX = x - lastX;
    double offsetY = y - lastY;
    lastX = x;
    lastY = y;

    offsetX *= sensitivity;
    offsetY *= sensitivity;

    auto& yaw = is_space_scene ? yaw_space : yaw_room;
    auto& pitch = is_space_scene ? pitch_space : pitch_room;

    yaw += offsetX;
    pitch -= offsetY;

    pitch = std::clamp(pitch, -89.0, 89.0);

    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    if (pressed) {
        auto& cam_front = is_space_scene ? cam_space_front : cam_room_front;
        cam_front = glm::normalize(dir);
    }

    first_move = !pressed;
}

void Application::on_mouse_button(int button, int action, int mods) {
    pressed = button == GLFW_MOUSE_BUTTON_2 && action == GLFW_PRESS;
}

void Application::on_key_pressed(int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
        w_hold = true;
    if (key == GLFW_KEY_W && action == GLFW_RELEASE)
        w_hold = false;

    if (key == GLFW_KEY_S && action == GLFW_PRESS)
        s_hold = true;
    if (key == GLFW_KEY_S && action == GLFW_RELEASE)
        s_hold = false;

    if (key == GLFW_KEY_A && action == GLFW_PRESS)
        speed *= 2;

    if (key == GLFW_KEY_D && action == GLFW_PRESS)
        speed /= 2;

    if (key == GLFW_KEY_Q && action == GLFW_PRESS && is_space_scene)
        show_menu = !show_menu;

    if (key == GLFW_KEY_E && action == GLFW_PRESS)
        is_space_scene = !is_space_scene;

    PV112Application::on_key_pressed(key, scancode, action, mods);
}
