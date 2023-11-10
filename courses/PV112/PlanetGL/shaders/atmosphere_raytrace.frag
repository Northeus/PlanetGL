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
layout(location = 4) uniform bool ignore_light = false;
layout(location = 5) uniform bool has_second_texture = false;

layout(binding = 3) uniform sampler2D albedo_texture;
layout(binding = 4) uniform sampler2D albedo_second_texture;

layout(location = 0) in vec3 fs_position;
layout(location = 1) in vec3 fs_normal;
layout(location = 2) in vec2 fs_texture_coordinate;

layout(location = 0) out vec4 final_color;

const vec3 EARTH_POSITION = vec3(0, 0, 0);
const float EARTH_RADIUS = 1.0f;
const float ATMOSPHERE_RADIUS = 1.2f;

// https://www.youtube.com/watch?v=HFPlKQGChpE
ray get_atmosphere_ray() {
    vec3 rd = normalize(fs_position - camera.position);

    float t = dot(earth_position - camera.position, rd);

    if ( t <= 0.0f ) {
        return ray(vec3(0.0f), vec3(0.0f));
    }

    vec3 p = camera.position + rd * t;
    float y = length(p - earth_position);

    if (y < atmos_radius) {
        float x = sqrt(atmos_radius * atmos_radius - y * y);
        float t1 = t - x;
        float t2 = t + x;

        return ray(camera.position + t1 * rd, camera.position + t2 * rd);
    }

    return ray(vec3(0.0f), vec3(0.0f));
}

ray get_earth_ray() {
    vec3 rd = normalize(fs_position - camera.position);

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

        if (t1 < 0.0f)
            return ray(camera
        else
            return ray(camera.position + t1 * rd, camera.position + t2 * rd);
    }

    return ray(vec3(0.0f), vec3(0.0f));
}

// https://www.youtube.com/watch?v=JMUtQcJE2Pw
const int step_count = 16;
vec3 sample_light_ray(
    vec3 sample_position,
    float earth_scale,
    float _earth_radius,
    float _total_radius,
    float reyleigh_scale,
    float mie_scale,
    float absorption_heigh_max,
    float absorption_falloff) {

}

vec3 compute_atmosphere_light(out vec3 scattering_color, out vec3 scattering_opacity) {
    // Prepare variables
    vec3 beta_reyleigh = vec3(5.5e-6, 13.0e-6, 22.4e-6);
    float beta_mie = 21e-6;
    vec3 beta_absorption = vec3(2.04e-5, 4.97e-5, 1.95e-6);
    float sun_intensity = 40.0;
    float g = 0.76;

    float _earth_radius = earth_radius;
    float _atmos_radius = atmos_radius - earth_radius;
    float _total_radius = _earth_radius + _atmos_radius;

    float ref_earth_radius = 6371000.0;
    float ref_atmos_radius = 100000.0;
    float ref_total_radius = ref_earth_radius + ref_atmos_radius;
    float ref_ratio = ref_earth_radius / ref_atmos_radius;

    float scale_ratio = _earth_radius / _atmos_radius;
    float earth_scale = ref_earth_radius / _earth_radius;
    float atmos_scale = scale_ratio / ref_ratio;

    float reyleigh_scale = 8500.0 / (earth_scale * atmos_scale);
    float mie_scale = 1200.0 / (earth_scale * atmos_scale);
    float absorption_heigh_max = 32000.0 * (earth_scale * atmos_scale);
    float absorption_falloff = 3000.0 / (earth_scale * atmos_scale);

    // Variables
    vec3 optical_depth = vec3(0.0f);

    // Compute
    ray atmo_ray = get_atmosphere_ray();
    ray earth_ray = get_earth_ray();
    ray comp_ray = atmo_ray;

    if (length(atmo_ray.to - atmo_ray.from) <= 0.0001f) {
        return vec3(0.0f);
    }

    if (length(earth_ray.to - earth_ray.from) >= 0.0001f) {
        comp_ray.to = earth_ray.from;
    }

    vec3 ray_vec = comp_ray.to - comp_ray.from;
    vec3 ray_dir = normalize(ray_vec);
    vec3 ray_step = ray_vec / step_count;
    vec3 ray_virt = length(ray_step) * earth_scale;

    vec3 sun_dir = normalize(light.position - camera.position);

    // Physics i guess
    float mu = dot(ray_dir, sun_dir);
    float mu2 = mu * mu;
    float g2 = g * g;
    float phase_rayleigh = 3.0 / (16.0 * PI) * (1.0 + mu2);
    float phase_mie = 3.0
        / (8.0 * PI) * ((1.0 - g2) * (mu2 + 1.0))
        / (pow(1.0 + g2 - 2.0 * mu * g, 1.5) * (2.0 + g2));

    for (int i = 0; i < step_count; i++) {
        vec3 sample_position = comp_ray.from + ray_step * i;

        float height = max(0.0, length(sample_position) - _earth_radius);

        float optical_depth_rayleigh = exp(-height / reyleigh_scale) * ray_virt;
        float optical_depth_mie = exp(-height / mie_scale) * ray_virt;

        float optical_depth_ozone =
            (1.0 / cosh((absorption_heigh_max - height) / absorption_falloff))
            * optical_depth_rayleigh * ray_virt;

        optical_depth += vec3(optical_depth_rayleigh, optical_depth_mie, optical_depth_ozone);

        vec3 optical_depth_light = sample_light_ray(
            sample_position, earth_scale, _earth_radius, _total_radius, reyleigh_scale,
            mie_scale, absorption_heigh_max, absorption_falloff);

        vec3 r = vec3(
            beta_reyleigh * (optical_depth.x + optical_depth_light.x)
            + beta_mie * (optical_depth.y + optical_depth_light.y)
            + beta_absorption * (optical_depth.z + optical_depth_light.z));
        vec3 attn = exp(-r);

        accumulated_reyleigh += optical_depth_rayleigh * attn;
        accumulated_mie += optical_depth_mie * attn;
    }

    scattering_color = sun_intensity * ( phase_rayleigh * beta_reyleigh * accumulated_reyleigh
                                        + phase_mie * beta_mie * accumulated_mie );
    scattering_opacity = exp(
        -(beta_mie * optical_depth.y
            + beta_reyleigh * optical_depth.x
            + beta_absorption * optical_depth.z);
}

void main() {
    if (ignore_light) {
        final_color = has_texture
            ? texture(albedo_texture, fs_texture_coordinate)
            : object.ambient_color;
    }
    else
    {
        vec3 light_vector = light.position.xyz - fs_position * light.position.w;
        vec3 L = normalize(light_vector);
        vec3 N = normalize(fs_normal);
        vec3 E = normalize(camera.position - fs_position);
        vec3 H = normalize(L + E);

        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0001);

        vec3 ambient = object.ambient_color.rgb
            * light.ambient_color.rgb
            * (has_texture ? texture(albedo_texture, fs_texture_coordinate).rgb : vec3(1.0));
        vec3 diffuse = object.diffuse_color.rgb
            * light.diffuse_color.rgb
            * (has_texture ? texture(albedo_texture, fs_texture_coordinate).rgb : vec3(1.0));
        vec3 specular = object.specular_color.rgb * light.specular_color.rgb;

        vec3 color = ambient + NdotL * diffuse + pow(NdotH, object.specular_color.w) * specular;

        if (has_second_texture) {
            vec3 light_diff = vec3(1.0f, 1.0f, 1.0f)
                - object.ambient_color.rgb * light.ambient_color.rgb;
            color += light_diff * texture(albedo_second_texture, fs_texture_coordinate).rgb;
        }

        // if (light.position.w == 1.0) {
        //     color /= (dot(light_vector, light_vector)) * 0.01f; // Lowered by x TODO
        // }

        // color = color / (color + 1.0);       // tone mapping
        // color = pow(color, vec3(1.0 / 2.2)); // gamma correction

        final_color = vec4(color, 1.0);
    }
}
