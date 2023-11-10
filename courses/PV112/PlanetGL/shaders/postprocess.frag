#version 450

layout(binding = 0, std140) uniform Camera {
    mat4 projection;
    mat4 view;
    vec3 position;
}
camera;

layout(location = 1) uniform mat4 projection;
layout(location = 2) uniform mat4 view;

const vec3 earth_position = vec3(0.0f);
const float earth_radius = 1.0f;

in vec2 UV;

out vec4 color;

uniform sampler2D renderTexture;

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

ray get_atmosphere_ray(vec3 rd) {
    float t = dot(earth_position - camera.position, rd);

    if ( t <= 0.0f ) {
        return ray(vec3(0.0f), vec3(0.0f));
    }

    vec3 p = camera.position + rd * t;
    float y = length(p - earth_position);

    if (y < earth_radius) {
        float x = sqrt(earth_radius * earth_radius - y * y);
        float t1 = t - x;
        float t2 = t + x;

        return ray(camera.position + t1 * rd, camera.position + t2 * rd);
    }

    return ray(vec3(0.0f), vec3(0.0f));
}

void main() {
    color = texture(renderTexture, UV);

    vec3 dir = get_ray();
    ray r = get_atmosphere_ray(dir);

    if (length(r.to - r.from) > 0.0f) {
        color = vec4(vec3(1.0f) - texture(renderTexture, UV).xyz, 1.0f);
    }
}
