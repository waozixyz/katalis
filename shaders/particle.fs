#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

// Output
out vec4 finalColor;

// Texture sampler
uniform sampler2D texture0;

void main() {
    vec4 texColor = texture(texture0, fragTexCoord);

    // Apply vertex color (includes alpha for fade)
    finalColor = texColor * fragColor;

    // Discard fully transparent pixels
    if (finalColor.a < 0.01) {
        discard;
    }
}
