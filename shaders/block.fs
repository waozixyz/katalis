#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec3 u_ambient_light;

void main() {
    vec4 texColor = texture(texture0, fragTexCoord);

    // Apply vertex color (from greedy meshing) and ambient light
    vec3 color = texColor.rgb * fragColor.rgb * u_ambient_light;

    finalColor = vec4(color, texColor.a);
}
