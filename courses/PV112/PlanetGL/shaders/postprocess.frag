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

layout(location = 1) uniform mat4 projection;
layout(location = 2) uniform mat4 view;
uniform sampler2D renderTexture;

layout(location = 3) uniform int number_of_measurements;
layout(location = 4) uniform int number_of_optical_depths;
layout(location = 5) uniform float density_falloff;
layout(location = 6) uniform vec3 wave_lengths;
layout(location = 7) uniform float scattering_strength;

const vec3 earth_position = vec3(0.0f, 0.0f, 1.0f);
const float earth_radius = 1.0f;
const float atmosphere_radius = 1.6;

in vec2 UV;

out vec4 color;

struct ray {
    vec3 from;
    vec3 to;
};

// https://stackoverflow.com/questions/71731722/correct-way-to-generate-3d-world-ray-from-2d-mouse-coordinates
vec3 get_ray() {
    vec2 nds = ( UV - vec2(0.5) ) * 2;

    vec3 ray_nds = vec3(nds, 1.0f);
    vec4 ray_clip = vec4(nds, -1.0f, 1.0f);

    vec4 ray_eye = inverse(projection) * ray_clip;

    ray_eye = vec4(ray_eye.xy, -1.0f, 0.0f);

    vec3 ray_wor = (inverse(view) * ray_eye).xyz;

    return normalize(ray_wor);
}

struct intersections {
    float t1;
    float t2;
    bool did;
};

intersections get_sphere_intersection_t(
    vec3 sphere_position, float sphere_radius,
    vec3 position, vec3 direction_normal) {
    float t = dot(sphere_position - position, direction_normal);

    vec3 sphere_intersection_midpoint = position + direction_normal * t;
    float distance_to_sphere_mid = length(sphere_intersection_midpoint - sphere_position);

    if (distance_to_sphere_mid < sphere_radius) {
        float half_intersect_length = sqrt(sphere_radius * sphere_radius
            - distance_to_sphere_mid * distance_to_sphere_mid);
        float t1 = t - half_intersect_length;
        float t2 = t + half_intersect_length;

        if (t1 < 0.0f && t2 < 0.0f) {
            return intersections(0.0f, 0.0f, false);
        }

        return intersections(min(t1, t2), max(t1, t2), true);
    }

    return intersections(0.0f, 0.0f, false);
}

float density_at_point(vec3 position) {
    float height_above_surface = length(position - earth_position) - earth_radius;
    float height_scaled = height_above_surface / (atmosphere_radius - earth_radius);
    float local_density = exp(-height_scaled * density_falloff) * (1 - height_scaled);

    return local_density;
}

float optical_depth(vec3 position, vec3 light_dir, float ray_length) {
     vec3 sample_point = position;
     float step_size = ray_length / (number_of_optical_depths - 1);
     float value = 0.0f;

     for (int i = 0; i < number_of_optical_depths; i++) {
        float local_density = density_at_point(sample_point);
        value += local_density * step_size;
        sample_point += light_dir * step_size;
     }

     return value;
}

vec3 calculate_light(vec3 position, vec3 direction_normal, float length, vec3 orig_color) {
    vec3 scatter_color = pow(400 / wave_lengths, vec3(4)) * scattering_strength;

    vec3 scatter_point = position;
    float step_size = length / (number_of_measurements - 1);
    vec3 scattered_light = vec3(0.0f);
    vec3 dir_to_sun = normalize(light.position.xyz - earth_position);
    float view_ray_optical_depth = 0;

    for (int i = 0; i < number_of_measurements; i++) {
        intersections sun_ray_intersections = get_sphere_intersection_t(
            earth_position, atmosphere_radius, scatter_point, dir_to_sun);
        float sun_ray_length = 1.0f;//sun_ray_ts.t2; // We are in atmosphere

        float sun_ray_optical_depth = optical_depth(
            scatter_point, dir_to_sun, sun_ray_length);
        view_ray_optical_depth = optical_depth(
            scatter_point, -direction_normal, step_size * i);
        vec3 transmittance =
            exp(-(sun_ray_optical_depth + view_ray_optical_depth) * scatter_color);
        float local_density = density_at_point(scatter_point);

        scattered_light += local_density * transmittance * scatter_color * step_size;
        scatter_point += direction_normal * step_size;
    }
    float original_color_transmittance = exp(-view_ray_optical_depth);

    return orig_color * original_color_transmittance + scattered_light;
}

void main() {
    color = texture(renderTexture, UV);
    vec3 ray_dir = get_ray();

    intersections earth_intersections = get_sphere_intersection_t(
        earth_position, earth_radius, camera.position, ray_dir);
    intersections atmosphere_intersections = get_sphere_intersection_t(
        earth_position, atmosphere_radius, camera.position, ray_dir);

    if (!atmosphere_intersections.did) {
        return;
    }

    float distance_to_atmosphere = atmosphere_intersections.t1;
    float distance_through_atmosphere =
        atmosphere_intersections.t2 - atmosphere_intersections.t1;

    if (atmosphere_intersections.t1 < 0.0f && atmosphere_intersections.t2 > 0.0f) {
        distance_to_atmosphere = 0;
        distance_through_atmosphere = atmosphere_intersections.t2;
    }

    if (earth_intersections.did) {
       distance_through_atmosphere = earth_intersections.t1 - distance_to_atmosphere;
    }

    vec3 point_in_atmosphere =
        camera.position + ray_dir * distance_to_atmosphere;
    vec3 light = calculate_light(
        point_in_atmosphere, ray_dir, distance_through_atmosphere, color.xyz);
    color = vec4(light, 1.0f);
}

