#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform vec3 u_sun_direction;
uniform float u_time_of_day;
uniform vec3 u_cam_forward;
uniform vec3 u_cam_right;
uniform vec3 u_cam_up;

// Hash function for procedural stars
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Second hash for twinkle variation
float hash2(vec2 p) {
    return fract(sin(dot(p, vec2(269.5, 183.3))) * 43758.5453);
}

// Smooth noise for clouds
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);  // Smoothstep

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Fractal Brownian Motion for natural cloud shapes
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }

    return value;
}

void main() {
    // Calculate world-space ray direction from screen coordinates
    // Map fragTexCoord from [0,1] to [-1,1] for proper ray direction
    vec2 ndc = fragTexCoord * 2.0 - 1.0;

    // Construct view ray using camera basis vectors
    // FOV approximation: use 0.8 as the tangent of half-FOV
    float fovScale = 0.8;
    vec3 rayDir = normalize(u_cam_forward + u_cam_right * ndc.x * fovScale + u_cam_up * ndc.y * fovScale);

    // Use ray Y component for vertical gradient (0 = horizon, 1 = zenith)
    float y = rayDir.y * 0.5 + 0.5;
    y = clamp(y, 0.0, 1.0);

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

    // Sun disc - use angular distance from sun direction
    float sunDist = acos(clamp(dot(rayDir, u_sun_direction), -1.0, 1.0));

    // Sun disc (visible when above horizon)
    if (u_sun_direction.y > -0.1) {
        // Core sun disc (angular size ~0.05 radians)
        float sunSize = 0.05;
        float sunDisc = 1.0 - smoothstep(sunSize * 0.8, sunSize, sunDist);

        // Sun color - white/yellow core
        vec3 sunColor = vec3(1.0, 0.95, 0.8);

        // Make sun more orange near horizon
        if (u_sun_direction.y < 0.3) {
            float horizonFactor = 1.0 - (u_sun_direction.y / 0.3);
            sunColor = mix(sunColor, vec3(1.0, 0.6, 0.2), horizonFactor * 0.7);
        }

        // Add sun glow
        float sunGlow = exp(-sunDist * 8.0) * 0.4;
        skyColor += sunColor * sunGlow * max(0.0, u_sun_direction.y + 0.1);

        // Add sun disc
        skyColor = mix(skyColor, sunColor, sunDisc * max(0.0, u_sun_direction.y + 0.1));
    }

    // Moon disc (visible when sun is below horizon)
    if (u_sun_direction.y < 0.2) {
        // Moon is opposite to sun
        vec3 moonDir = -u_sun_direction;
        float moonDist = acos(clamp(dot(rayDir, moonDir), -1.0, 1.0));

        float moonSize = 0.04;
        float moonDisc = 1.0 - smoothstep(moonSize * 0.9, moonSize, moonDist);

        // Moon color - pale white/blue
        vec3 moonColor = vec3(0.9, 0.92, 1.0);

        // Moon visibility increases as sun goes down
        float moonVisibility = smoothstep(0.2, -0.1, u_sun_direction.y);

        // Subtle moon glow
        float moonGlow = exp(-moonDist * 10.0) * 0.15 * moonVisibility;
        skyColor += vec3(0.7, 0.75, 0.9) * moonGlow;

        // Add moon disc
        skyColor = mix(skyColor, moonColor, moonDisc * moonVisibility);
    }

    // Add sun glow near horizon during sunset/sunrise
    if (sunHeight < 0.5 && sunHeight > 0.1) {
        float glowIntensity = (0.5 - sunHeight) * 2.0;
        float horizonGlow = (1.0 - y) * glowIntensity;
        skyColor += vec3(1.0, 0.4, 0.1) * horizonGlow * 0.5;
    }

    // Procedural clouds (daytime, above horizon)
    if (sunHeight > 0.2 && rayDir.y > 0.1) {
        // Use world-space XZ for cloud UV (moves with camera)
        // Project ray onto cloud plane at fixed height
        vec2 cloudUV = rayDir.xz / max(rayDir.y, 0.1) * 2.0 + vec2(u_time_of_day * 0.01, 0.0);

        // Generate cloud density using FBM
        float cloudDensity = fbm(cloudUV);

        // Threshold for cloud visibility
        cloudDensity = smoothstep(0.4, 0.7, cloudDensity);

        // Fade clouds near horizon
        float cloudFade = smoothstep(0.1, 0.3, rayDir.y);

        // Cloud color (white, lit by sun)
        vec3 cloudColor = vec3(1.0);

        // Add subtle orange tint during sunset
        if (sunHeight < 0.4) {
            float sunsetTint = 1.0 - (sunHeight / 0.4);
            cloudColor = mix(cloudColor, vec3(1.0, 0.8, 0.6), sunsetTint * 0.5);
        }

        // Blend clouds into sky
        skyColor = mix(skyColor, cloudColor, cloudDensity * cloudFade * 0.5 * sunHeight);
    }

    // Procedural stars at night
    if (sunHeight < 0.4) {
        // Star visibility increases as sun goes down
        float starVisibility = 1.0 - (sunHeight / 0.4);
        starVisibility = starVisibility * starVisibility;  // Ease in

        // Only show stars above horizon
        if (rayDir.y > 0.0) {
            // Use spherical mapping for star UV (moves with camera)
            vec2 starUV = vec2(atan(rayDir.z, rayDir.x), asin(rayDir.y)) * 25.0;
            vec2 starCell = floor(starUV);
            vec2 starPos = fract(starUV);

            // Random star position within cell
            float starRand = hash(starCell);
            vec2 starOffset = vec2(hash(starCell + vec2(1.0, 0.0)), hash(starCell + vec2(0.0, 1.0)));

            // Distance from star center
            float dist = length(starPos - starOffset);

            // Star threshold (fewer stars = more sparse)
            if (starRand > 0.95) {
                // Star brightness with twinkle
                float twinkle = sin(u_time_of_day * 50.0 + hash2(starCell) * 100.0) * 0.3 + 0.7;
                float starBrightness = (1.0 - smoothstep(0.0, 0.1, dist)) * twinkle;

                // Vary star colors slightly
                vec3 starColor = vec3(1.0);
                float colorVar = hash2(starCell + vec2(5.0, 7.0));
                if (colorVar > 0.8) {
                    starColor = vec3(1.0, 0.9, 0.8);  // Warm star
                } else if (colorVar < 0.2) {
                    starColor = vec3(0.8, 0.9, 1.0);  // Cool star
                }

                skyColor += starColor * starBrightness * starVisibility * rayDir.y;
            }
        }
    }

    finalColor = vec4(skyColor, 1.0);
}
