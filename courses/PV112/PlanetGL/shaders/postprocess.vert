#version 450

// Output data ; will be interpolated for each fragment.
out vec2 UV;

const vec2 coords[6] = {
    vec2(-1.0f, -1.0f),
    vec2( 1.0f, -1.0f),
    vec2(-1.0f,  1.0f),
    vec2(-1.0f,  1.0f),
    vec2( 1.0f, -1.0f),
    vec2( 1.0f,  1.0f)
};

void main(){
	gl_Position = vec4(coords[gl_VertexID], 0.0f, 1.0f);
	UV = (coords[gl_VertexID] + vec2(1, 1)) / 2.0f;
}

