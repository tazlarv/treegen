#version 330 core

in vs_output {
  vec3 world_pos;
  vec2 tex_coords;
  vec3 normal;
} vs_out;

in float flogz;

out vec4 frag_color;

struct light_prop {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct material_prop {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform float farPlaneCoef;

uniform vec3 light_pos;
uniform vec3 view_pos;

uniform light_prop light;
uniform material_prop material;

uniform bool use_diffuse_map;
uniform bool use_normal_map;

uniform sampler2D diffuse_map;
uniform sampler2D normal_map;

void main() {
    // Blinn-Phong model

    float diff, spec;
    vec3 ambient, diffuse, specular;
    vec3 P, N, L, V, H;

    P = vs_out.world_pos;
    N = normalize(vs_out.normal);
    L = normalize(light_pos - P);
    V = normalize(view_pos - P);

    H = normalize(L + V);
    diff = dot(N, L);
    spec = pow(max(dot(N, H), 0.0), material.shininess);

    if (diff < 0.0) spec = 0.0;
    diff = max(diff, 0.0);

    if (use_diffuse_map) {
        vec3 tex_color = texture(diffuse_map, vs_out.tex_coords).rgb;
        ambient  = tex_color * light.ambient;
        diffuse  = tex_color * light.diffuse * diff;
        // Specular not used for textures
        specular = vec3(0.0, 0.0, 0.0);
    }
    else {  // Use material properties for color
        ambient  = material.ambient * light.ambient;
        diffuse  = material.diffuse * light.diffuse * diff;
        specular = material.specular * light.specular * spec;
    }
  	
    frag_color.rgb = ambient + diffuse + specular;
    frag_color.a   = 1.0;

    gl_FragDepth = log2(flogz) * (farPlaneCoef * 0.5);
};