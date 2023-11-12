#version 450

layout(binding = 0, std140) uniform Camera {
    mat4 projection;
    mat4 view;
    vec3 position;
}
camera;

layout(binding = 1, std140) uniform Light {
    vec4 position;
    vec4 ambient_color;
    vec4 diffuse_color;
    vec4 specular_color;
}
light;

layout(binding = 2, std140) uniform Object {
    mat4 model_matrix;

    vec4 ambient_color;
    vec4 diffuse_color;
    vec4 specular_color;
}
object;

layout(location = 3) uniform bool has_texture = false;

layout(binding = 3) uniform sampler2D albedo_texture;

layout(location = 0) in vec3 fs_position;
layout(location = 1) in vec3 fs_normal;
layout(location = 2) in vec2 fs_texture_coordinate;

layout(location = 0) out vec4 final_color;

void main() {
    vec3 light_vector = light.position.xyz - fs_position * light.position.w;
    vec3 L = normalize(light_vector);
    vec3 N = normalize(fs_normal);
    vec3 E = normalize(camera.position - fs_position);
    vec3 H = normalize(L + E);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0001);

    vec3 ambient = object.ambient_color.rgb
        * (has_texture ? texture(albedo_texture, fs_texture_coordinate).rgb : vec3(1.0))
        * light.ambient_color.rgb;
    vec3 diffuse = object.diffuse_color.rgb
        * (has_texture ? texture(albedo_texture, fs_texture_coordinate).rgb : vec3(1.0))
        * light.diffuse_color.rgb;
    vec3 specular = object.specular_color.rgb * light.specular_color.rgb;

    vec3 color = ambient.rgb + NdotL * diffuse.rgb + pow(NdotH, object.specular_color.w) * specular;

    //if (light.position.w == 1.0) {
    //    color /= (dot(light_vector, light_vector));
    //}

    // color = color / (color + 1.0);       // tone mapping
    // color = pow(color, vec3(1.0 / 2.2)); // gamma correction

    final_color = vec4(color, 1.0);
}
