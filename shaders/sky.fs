#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform vec3 u_sun_direction;
uniform float u_time_of_day;

void main() {
    // Use texture coordinates as view direction approximation
    // fragTexCoord goes from (0,0) bottom-left to (1,1) top-right
    float y = fragTexCoord.y;  // 0 = bottom (horizon), 1 = top (zenith)

    // Sun height factor (0 = below horizon, 1 = directly overhead)
    float sunHeight = u_sun_direction.y * 0.5 + 0.5;

    // Zenith color (top of sky)
    vec3 zenithDay = vec3(0.4, 0.6, 1.0);      // Bright blue
    vec3 zenithNight = vec3(0.02, 0.02, 0.08); // Dark blue
    vec3 zenithColor = mix(zenithNight, zenithDay, sunHeight);

    // Horizon color
    vec3 horizonDay = vec3(0.8, 0.85, 0.95);   // Light blue/white
    vec3 horizonNight = vec3(0.05, 0.05, 0.1); // Dark
    vec3 horizonSunset = vec3(1.0, 0.5, 0.2);  // Orange

    // Blend horizon color based on sun position
    vec3 horizonColor = mix(horizonNight, horizonDay, sunHeight);

    // Add sunset/sunrise colors when sun is near horizon
    float sunsetFactor = 1.0 - abs(u_sun_direction.y);
    sunsetFactor = sunsetFactor * sunsetFactor * sunsetFactor;  // Make it more concentrated
    horizonColor = mix(horizonColor, horizonSunset, sunsetFactor * 0.7);

    // Blend between horizon and zenith based on vertical position
    // Use a curve to make horizon color extend higher
    float blend = pow(y, 0.5);  // Square root for smoother gradient
    vec3 skyColor = mix(horizonColor, zenithColor, blend);

    // Add sun glow near horizon during sunset/sunrise
    if (sunHeight < 0.5 && sunHeight > 0.1) {
        float glowIntensity = (0.5 - sunHeight) * 2.0;
        float horizonGlow = (1.0 - y) * glowIntensity;
        skyColor += vec3(1.0, 0.4, 0.1) * horizonGlow * 0.5;
    }

    finalColor = vec4(skyColor, 1.0);
}
