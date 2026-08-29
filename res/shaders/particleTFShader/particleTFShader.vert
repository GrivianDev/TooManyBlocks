#version 430

#include "../include/random.glsl"

#define MAX_MODULES 100

#define SPAWNMODULE_FLAG         (1 << 0)
#define SPAWNLOCATIONMODULE_FLAG (1 << 1)
#define INITMODULE_FLAG          (1 << 2)
#define UPDATEMODULE_FLAG        (1 << 3)

#define DYNAMIC_SPAWNRATE        (1 << 0)

#define TEXINDEX_BITMASK  0xFFFF
#define TEXINDEX_OFFSET   16

#define SET_BITS(target, value, bitmask, position) (target = (target & ~(bitmask << position)) | ((value & bitmask) << position))
#define GET_BITS(target, bitmask, position) ((target >> position) & bitmask)

#define SpawnFixedParticleCount 0U
#define SpawnRate               1U
#define SpawnBurst              2U

// Spawn location modules
#define PointSpawn  3U
#define BoxSpawn    4U
#define SphereSpawn 5U
#define ConeSpawn   6U
#define DiskSpawn   7U
#define LineSpawn   8U

// Initializaion modules
#define InitialVelocity         9U
#define InitialVelocityInCone   10U
#define InitialLifetime         11U
#define InitialSize             12U
#define InitialColor            13U
#define InitialAlpha            14U
#define InitialTexture          15U

// Update modules
#define Drag            16U
#define Acceleration    17U
#define Turbulence      18U
#define SizeOverLife    19U
#define ColorOverLife   20U
#define AlphaOverLife   21U
#define AnimatedTexture 22U

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec3 in_velocity;
layout(location = 2) in vec3 in_position;
layout(location = 3) in float in_timeToLive;
layout(location = 4) in float in_initialTimeToLive;
layout(location = 5) in float in_size;
layout(location = 6) in uint in_metadata;

layout(location = 0) out vec4 tf_color;
layout(location = 1) out vec3 tf_velocity;
layout(location = 2) out vec3 tf_position;
layout(location = 3) out float tf_timeToLive;
layout(location = 4) out float tf_initialTimeToLive;
layout(location = 5) out float tf_size;
layout(location = 6) out uint tf_metadata;

struct ParticleModule {
    uint type;
    uint flags;
    uint metadata1;
    uint metadata2;
    vec4 params[5];
};

layout(std140) uniform ParticleModulesBlock {
    ParticleModule u_modules[MAX_MODULES];
};

uniform uint u_moduleCount;
uniform uint u_spawnCount;
uniform uint u_particleSpawnOffset;
uniform uint u_allocatedParticleCount;
uniform uint u_flags;
uniform float u_deltaTime;
uniform float u_time;

void copyParticle() {
    tf_color = in_color;
    tf_velocity = in_velocity;
    tf_position = in_position;
    tf_timeToLive = in_timeToLive;
    tf_initialTimeToLive = in_initialTimeToLive;
    tf_size = in_size;
    tf_metadata = in_metadata;
}

void spawnParticle() {
    uint seed = floatBitsToUint(u_time) + gl_VertexID;

    tf_color = vec4(1.0);
    tf_velocity = vec3(0.0);
    tf_position = vec3(0.0);
    tf_timeToLive = 0.0;
    tf_initialTimeToLive = 0.0;
    tf_size = 1.0;
    tf_metadata = 0U;

    for (uint i = 0; i < u_moduleCount; i++) {
        ParticleModule module = u_modules[i];
        if ((module.flags & SPAWNLOCATIONMODULE_FLAG) != 0) {
            switch (module.type) {
            case PointSpawn:
                tf_position = vec3(0);
                break;
            case BoxSpawn: {
                tf_position = vec3(
                    mix(module.params[0].x, module.params[1].x, rand01(seed)),
                    mix(module.params[0].y, module.params[1].y, rand01(seed)),
                    mix(module.params[0].z, module.params[1].z, rand01(seed))
                );
                break;
            }
            case SphereSpawn: {
                vec3 randomDir = normalize(vec3(
                    rand01(seed) * 2.0 - 1.0,
                    rand01(seed) * 2.0 - 1.0,
                    rand01(seed) * 2.0 - 1.0
                ));
                tf_position = randomDir * mix(module.params[1].x, module.params[0].x, rand01(seed));
                break;
            }
            case ConeSpawn: {
                float height = module.params[0].x;
                float radius = module.params[1].x;

                // Generate random height along the cone
                float randHeight = rand01(seed) * height;
                float localRadius = (randHeight / height) * radius;

                // Random angle around the cone axis
                float theta = rand01(seed) * 6.2831853; // 2π

                // Sample point on the circle at current height
                float r = rand01(seed) * localRadius;

                tf_position = vec3(
                    r * cos(theta),
                    randHeight,
                    r * sin(theta)
                );
                break;
            }
            case DiskSpawn: {
                vec3 randomDir = normalize(vec3(
                    rand01(seed) * 2.0 - 1.0,
                    0.0,
                    rand01(seed) * 2.0 - 1.0
                ));
                tf_position = randomDir * mix(module.params[1].x, module.params[0].x, rand01(seed));
                break;
            }
            case LineSpawn: {
                tf_position = vec3(mix(module.params[0], module.params[1], rand01(seed)));
                break;
            }
            }
        } else if((module.flags & INITMODULE_FLAG) != 0) {
            switch (module.type) {
            case InitialVelocity: {
                tf_velocity = vec3(
                    mix(module.params[0].x, module.params[1].x, rand01(seed)),
                    mix(module.params[0].y, module.params[1].y, rand01(seed)),
                    mix(module.params[0].z, module.params[1].z, rand01(seed))
                );
                break;
            }
            case InitialVelocityInCone: {
                vec3 axis = module.params[2].xyz;

                float outerAngle = module.params[3].x;
                float innerAngle = module.params[4].x;

                // Generate a random angle between inner and outer cone
                float angle = mix(innerAngle, outerAngle, rand01(seed));

                // Sample random direction inside cone (spherical cap)
                float z = cos(radians(angle)); // direction along axis
                float sinTheta = sqrt(1.0 - z * z);

                float phi = rand01(seed) * 6.2831853; // random azimuth
                float x = cos(phi) * sinTheta;
                float y = sin(phi) * sinTheta;

                // Create direction in cone space (aligned to +Z)
                vec3 dir = vec3(x, y, z);

                // Rotate to align with the actual axis direction
                vec3 up = vec3(0.0, 0.0, 1.0);
                vec3 rotationAxis = cross(up, axis);
                float rotationAngle = acos(clamp(dot(up, axis), -1.0, 1.0));

                // Apply rotation if needed
                if (length(rotationAxis) > 0.0001) {
                    rotationAxis = normalize(rotationAxis);
                    float cosA = cos(rotationAngle);
                    float sinA = sin(rotationAngle);

                    // Rodrigues' rotation formula
                    dir = dir * cosA +
                        cross(rotationAxis, dir) * sinA +
                        rotationAxis * dot(rotationAxis, dir) * (1.0 - cosA);
                } else if (dot(up, axis) < 0.0) {
                    // Special case: axis is opposite of up
                    dir = -dir;
                }

                tf_velocity = dir * mix(module.params[0].x, module.params[1].x, rand01(seed));
                break;
            }
            case InitialLifetime: {
                tf_timeToLive = mix(module.params[0].x, module.params[1].x, rand01(seed));
                tf_initialTimeToLive = tf_timeToLive;
                break;
            }
            case InitialSize: {
                tf_size = mix(module.params[0].x, module.params[1].x, rand01(seed));
                break;
            }
            case InitialColor: {
                tf_color.rgb = vec3(
                    mix(module.params[0].x, module.params[1].x, rand01(seed)),
                    mix(module.params[0].y, module.params[1].y, rand01(seed)),
                    mix(module.params[0].z, module.params[1].z, rand01(seed))
                );
                break;
            }
            case InitialAlpha: {
                tf_color.a = module.params[0].x;
                break;
            }
            case InitialTexture: {
                SET_BITS(tf_metadata, module.metadata1, TEXINDEX_BITMASK, TEXINDEX_OFFSET);
                break;
            }
            }
        }
    }
}

void updateParticle() {
    uint seed = floatBitsToUint(u_time) + gl_VertexID;

    tf_position = in_position + in_velocity * u_deltaTime;
    tf_timeToLive = in_timeToLive - u_deltaTime;
    
    if (tf_timeToLive <= 0) {
        return;
    }

    float dragCoefficient = 0.0;
    vec3 jitterVel = vec3(0.0);

    for (uint i = 0; i < u_moduleCount; i++) {
        ParticleModule module = u_modules[i];
        if ((module.flags & UPDATEMODULE_FLAG) != 0) {
            switch (module.type) {
            case Drag: {
                dragCoefficient = module.params[0].x;
                break;
            }
            case Acceleration: {
                tf_velocity = in_velocity + module.params[0].xyz * u_deltaTime;
                break;
            }
            case Turbulence: {
                jitterVel = normalize(vec3(
                    rand01(seed) * 2.0 - 1.0,
                    rand01(seed) * 2.0 - 1.0,
                    rand01(seed) * 2.0 - 1.0
                )) * module.params[0].x * u_deltaTime;
                break;
            }
            case SizeOverLife: {
                uint numKeyframes = module.metadata1;
                if (numKeyframes > 0) {
                    float t = clamp(1.0 - in_timeToLive / in_initialTimeToLive, 0.0, 1.0);

                    // Extract the first and last time values
                    float t_first = module.params[0][0];
                    float t_last = module.params[0][numKeyframes - 1];

                    if (t <= t_first) {
                        tf_size = module.params[1].x; // Snap to first value
                    } else if (t >= t_last) {
                        tf_size = module.params[1].x; // Snap to last value
                    } else {
                        for (uint i = 0; i < numKeyframes - 1; i++) {
                            float t0 = module.params[0][i];
                            float t1 = module.params[0][i + 1];

                            if (t >= t0 && t <= t1) {
                                float s0 = module.params[i + 1].x;
                                float s1 = module.params[i + 2].x;

                                float localT = (t - t0) / max(t1 - t0, 1e-5);
                                tf_size = mix(s0, s1, localT);
                                break;
                            }
                        }
                    }
                }
                break;
            }
            case ColorOverLife: {
                uint numKeyframes = module.metadata1;
                if (numKeyframes > 0) {
                    float t = clamp(1.0 - in_timeToLive / in_initialTimeToLive, 0.0, 1.0);

                    // Extract the first and last time values
                    float t_first = module.params[0][0];
                    float t_last = module.params[0][numKeyframes - 1];

                    if (t <= t_first) {
                        tf_color.rgb = module.params[1].rgb; // Snap to first value
                    } else if (t >= t_last) {
                        tf_color.rgb = module.params[1].rgb; // Snap to last value
                    } else {
                        // Interpolate
                        for (uint i = 0; i < numKeyframes - 1; i++) {
                            float t0 = module.params[0][i];
                            float t1 = module.params[0][i + 1];

                            if (t >= t0 && t <= t1) {
                                vec3 c0 = module.params[i + 1].rgb;
                                vec3 c1 = module.params[i + 2].rgb;

                                float localT = (t - t0) / max(t1 - t0, 1e-5);
                                tf_color.rgb = mix(c0, c1, localT);
                                break;
                            }
                        }
                    }
                }
                break;
            }
            case AlphaOverLife: {
                uint numKeyframes = module.metadata1;
                if (numKeyframes > 0) {
                    float t = clamp(1.0 - in_timeToLive / in_initialTimeToLive, 0.0, 1.0);

                    // Extract the first and last time values
                    float t_first = module.params[0][0];
                    float t_last = module.params[0][numKeyframes - 1];

                    if (t <= t_first) {
                        tf_color.a = module.params[1].x; // Snap to first value
                    } else if (t >= t_last) {
                        tf_color.a = module.params[1].x; // Snap to last value
                    } else {
                        for (uint i = 0; i < numKeyframes - 1; i++) {
                            float t0 = module.params[0][i];
                            float t1 = module.params[0][i + 1];

                            if (t >= t0 && t <= t1) {
                                float a0 = module.params[i + 1].x;
                                float a1 = module.params[i + 2].x;

                                float localT = (t - t0) / max(t1 - t0, 1e-5);
                                tf_color.a = mix(a0, a1, localT);
                                break;
                            }
                        }
                    }
                }
                break;
            }
            case AnimatedTexture: {
                float offset = 0.0;
                uint baseTexIndex = module.metadata1;
                uint numFrames = module.metadata2;
                float fps = module.params[0].x;
                float offsetScale = module.params[1].x;
                if (offsetScale > 0.0) {
                    uint startSeed = gl_VertexID;
                    offset = rand01(startSeed) * offsetScale * (float(numFrames) / fps);
                }
                float age = 1.0 - (tf_timeToLive / (tf_initialTimeToLive + 0.0001));
                float elapsedRelativeTime = age * tf_initialTimeToLive;
                uint frameIndex = uint(floor((elapsedRelativeTime + offset) * fps)) % numFrames;
                SET_BITS(tf_metadata, baseTexIndex + frameIndex, TEXINDEX_BITMASK, TEXINDEX_OFFSET);
                break;
            }
            }
        }
    }

    // Apply drag
    tf_velocity *= max(1.0 - dragCoefficient * u_deltaTime, 0.0);
    // Apply turbulence
    tf_velocity += jitterVel;
}

void main() {
    copyParticle();
    if (in_timeToLive <= 0.0) {        
        if ((u_flags & DYNAMIC_SPAWNRATE) != 0) {
            if (u_spawnCount == 0) {
                return;
            }

            // Spawn based on free ringbuffer slots
            uint begin = u_particleSpawnOffset;
            uint end = (u_particleSpawnOffset + u_spawnCount - 1) % u_allocatedParticleCount;
            
            if (begin <= end && (gl_VertexID >= begin && gl_VertexID <= end)) {
                // Normal case, no wrap
                spawnParticle();
            } else if (begin > end && (gl_VertexID >= begin || gl_VertexID <= end)) {
                // Wrapped case
                spawnParticle();
            }
        } else {
            // Force respawn all
            spawnParticle();
        }        
    } else {
        updateParticle();
    }        
}