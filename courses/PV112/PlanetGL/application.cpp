#include "application.hpp"
#include "cube.hpp"
#include "geometry.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <glm/gtx/transform.hpp>
#include <memory>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

GLuint load_texture_2d(const std::filesystem::path filename) {
    int width, height, channels;
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

Application::Application(int initial_width, int initial_height, std::vector<std::string> arguments)
    : PV112Application(initial_width, initial_height, arguments) {
    this->width = initial_width;
    this->height = initial_height;

    // --------------------------------------------------------------------------
    //  Load/Create Objects
    // --------------------------------------------------------------------------
    skybox_texture = load_texture_2d(images_path / "stars.jpg");
    earth_day_texture = load_texture_2d(images_path / "earth_day.jpg");
    earth_night_texture = load_texture_2d(images_path / "earth_night.jpg");
    sun_texture = load_texture_2d(images_path / "sun.jpg");

    // --------------------------------------------------------------------------
    // Initialize UBO Data
    // --------------------------------------------------------------------------
    camera_ubo.projection = glm::perspective(
        glm::radians(45.0f),
        float(width) / float(height),
        0.01f, 1000.0f);
    camera_ubo.position = glm::vec4(camera.get_eye_position(), 1.0f);
    camera_ubo.view = glm::lookAt(
        glm::vec3(camera_ubo.position),
        cam_front,
        glm::vec3(0.0f, 1.0f, 0.0f));

    // TODO make again point (pos.w = 1)
    light_ubo.position = glm::vec4(0.0f, 6.0f, 18.0f, 0.0f);
    light_ubo.ambient_color = glm::vec4(1.0f);
    light_ubo.diffuse_color = glm::vec4(1.0f);
    light_ubo.specular_color = glm::vec4(1.0f);

    skybox_ubo.model_matrix = glm::scale(glm::vec3{100.0f, 100.0f, 100.0f});
    skybox_ubo.ambient_color = glm::vec4(0.05f);
    skybox_ubo.diffuse_color = glm::vec4(0.0f);
    skybox_ubo.specular_color = glm::vec4(0.0f);

    // TODO update value
    earth_ubo.model_matrix = glm::translate(
        glm::scale(
            glm::rotate(glm::radians(180.0f),
                glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::vec3{1.0f, 1.0f, 1.0f}),
        glm::vec3(0.0f, 0.0f, 1.0f));
    earth_ubo.ambient_color = glm::vec4(0.0f);
    earth_ubo.diffuse_color = glm::vec4(0.3f);
    earth_ubo.specular_color = glm::vec4(0.0f);

    sun_ubo.model_matrix = glm::translate(
        glm::scale(glm::vec3{5.0f, 5.0f, 5.0f}),
        glm::vec3(light_ubo.position));
    sun_ubo.ambient_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    sun_ubo.diffuse_color = glm::vec4(0.0f);
    sun_ubo.specular_color = glm::vec4(0.0f);

    // --------------------------------------------------------------------------
    // Create Buffers
    // --------------------------------------------------------------------------
    glCreateBuffers(1, &camera_buffer);
    glNamedBufferStorage(camera_buffer, sizeof(CameraUBO), &camera_ubo, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &light_buffer);
    glNamedBufferStorage(light_buffer, sizeof(LightUBO), &light_ubo, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &skybox_buffer);
    glNamedBufferStorage(
        skybox_buffer,
        sizeof(ObjectUBO),
        &skybox_ubo,
        0);

    glCreateBuffers(1, &earth_buffer);
    glNamedBufferStorage(
        earth_buffer,
        sizeof(ObjectUBO),
        &earth_ubo,
        GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &sun_buffer);
    glNamedBufferStorage(
        sun_buffer,
        sizeof(ObjectUBO),
        &sun_ubo,
        0);

    glCreateFramebuffers(1, &frame_buffer_name);
    glCreateTextures(GL_TEXTURE_2D, 1, &render_texture);
    glGenRenderbuffers(1, &depth_buffer);

    // TODO push this to initialization and use proper functions (from sem)
    glCreateVertexArrays(1, &render_texture_vao);

    compile_shaders();
}

Application::~Application() {
    delete_shaders();

    // TODO

    glDeleteBuffers(1, &camera_buffer);
    glDeleteBuffers(1, &light_buffer);
    glDeleteBuffers(1, &skybox_buffer);
}

// ----------------------------------------------------------------------------
// Methods
// ----------------------------------------------------------------------------

void Application::delete_shaders() {}

void Application::compile_shaders() {
    delete_shaders();
    normal_program = create_program(
        lecture_shaders_path / "normal.vert",
        lecture_shaders_path / "normal.frag");

    postprocess_program = create_program(
        lecture_shaders_path / "postprocess.vert",
        lecture_shaders_path / "postprocess.frag");
}

void Application::update(float delta) {
    if (w_hold)
        camera_ubo.position += glm::vec4(cam_front, 0.0f) * speed * delta;
    if (s_hold)
        camera_ubo.position -= glm::vec4(cam_front, 0.0f) * speed * delta;

    earth_ubo.model_matrix = glm::rotate(
        earth_ubo.model_matrix,
        glm::radians(360.0f * (delta / 10000.0f)),
        glm::vec3(0.0f, 1.0f, 0.0f));

/* TODO uncomment rotation
    glNamedBufferSubData(
        earth_buffer,
        0,
        sizeof(ObjectUBO),
        &earth_ubo);
*/
}

void Application::render() {
    // --------------------------------------------------------------------------
    // Set up framebuffer
    // --------------------------------------------------------------------------
    // Setup framebuffer (TODO should it be all there?)
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_name);

    glBindTexture(GL_TEXTURE_2D, render_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, render_texture, 0);

    GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, draw_buffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Cannot initialize frame buffer\n";
        throw std::exception{};
    }

    // --------------------------------------------------------------------------
    // Update UBOs
    // --------------------------------------------------------------------------
    // Camera
    /* TODO?
    camera_ubo.position = glm::vec4(camera.get_eye_position(), 0.0f);
    camera_ubo.projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(width) / static_cast<float>(height),
        0.01f, 1000.0f);
    camera_ubo.view = glm::lookAt(
        camera.get_eye_position(),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    */

    camera_ubo.view = glm::lookAt(
        glm::vec3(camera_ubo.position),
        glm::vec3(camera_ubo.position) + cam_front,
        glm::vec3(0.0f, 1.0f, 0.0f));

    glNamedBufferSubData(camera_buffer, 0, sizeof(CameraUBO), &camera_ubo);

    // --------------------------------------------------------------------------
    // Draw scene
    // --------------------------------------------------------------------------
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    // Clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Configure fixed function pipeline
    glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Prepare shader
    glUseProgram(normal_program);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, camera_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, light_buffer);

    // Draw sun
    glUniform1i(glGetUniformLocation(normal_program, "has_texture"), true);
    glBindTextureUnit(3, sun_texture);
    glBindBufferRange(GL_UNIFORM_BUFFER, 2, sun_buffer, 0, sizeof(ObjectUBO));
    unit_sphere.draw();

    // Draw earth
    glUniform1i(glGetUniformLocation(normal_program, "has_texture"), true);
    glBindTextureUnit(3, earth_day_texture);
    glBindBufferRange(GL_UNIFORM_BUFFER, 2, earth_buffer, 0, sizeof(ObjectUBO));
    unit_sphere.draw();

    // Draw skybox
    glUniform1i(glGetUniformLocation(normal_program, "has_texture"), true);
    glBindTextureUnit(3, skybox_texture);
    glDisable(GL_CULL_FACE);
    glBindBufferRange(GL_UNIFORM_BUFFER, 2, skybox_buffer, 0, sizeof(ObjectUBO));
    // unit_cube.draw();

    // TODO CITE:
    // http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
    // ----------a----------------------------------------------------------------
    // Set up framebuffer
    // --------------------------------------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(postprocess_program);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, camera_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, light_buffer);
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(camera_ubo.projection));
    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(camera_ubo.view));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, render_texture);

    glBindVertexArray(render_texture_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

}

void Application::render_ui() { const float unit = ImGui::GetFontSize(); }

// ----------------------------------------------------------------------------
// Input Events
// ----------------------------------------------------------------------------
void Application::on_resize(int width, int height) {
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

    yaw += offsetX;
    pitch -= offsetY;

    pitch = std::clamp(pitch, -89.0, 89.0);

    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    if (pressed) {
        cam_front = glm::normalize(dir);
    }

    first_move = !pressed;
    // camera.on_mouse_move(x, y);
}

void Application::on_mouse_button(int button, int action, int mods) {
    pressed = button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS;
    // camera.on_mouse_button(button, action, mods);
}

void Application::on_key_pressed(int key, int scancode, int action, int mods) {
    // Calls default implementation that invokes compile_shaders when 'R key is hit.

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

    PV112Application::on_key_pressed(key, scancode, action, mods);
}
