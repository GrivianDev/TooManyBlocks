#define MAX_MODULES 100

#define SPAWNMODULE_FLAG         (1u << 0u)
#define SPAWNLOCATIONMODULE_FLAG (1u << 1u)
#define INITMODULE_FLAG          (1u << 2u)
#define UPDATEMODULE_FLAG        (1u << 3u)

#define DYNAMIC_SPAWNRATE        (1u << 0u)

#define TEXINDEX_BITMASK  0xFFFFu
#define TEXINDEX_OFFSET   16u

#define SET_BITS(target, value, bitmask, position) (target = (target & ~(bitmask << position)) | ((value & bitmask) << position))
#define GET_BITS(target, bitmask, position) ((target >> position) & bitmask)

#define SpawnFixedParticleCount 0u
#define SpawnRate               1u
#define SpawnBurst              2u

// Spawn location modules
#define PointSpawn  3u
#define BoxSpawn    4u
#define SphereSpawn 5u
#define ConeSpawn   6u
#define DiskSpawn   7u
#define LineSpawn   8u

// Initializaion modules
#define InitialVelocity         9u
#define InitialVelocityInCone   10u
#define InitialLifetime         11u
#define InitialSize             12u
#define InitialColor            13u
#define InitialAlpha            14u
#define InitialTexture          15u

// Update modules
#define Drag            16u
#define Acceleration    17u
#define Turbulence      18u
#define SizeOverLife    19u
#define ColorOverLife   20u
#define AlphaOverLife   21u
#define AnimatedTexture 22u

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

uint _pcg_hash(uint seedState) {
    uint state = seedState * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float _rand01(inout uint seedState) {
    const float UINT_MAX_FLOAT = 4294967295.0;
    seedState++; // Modify passed seed to ensure random numbers on multiple calls
    return float(_pcg_hash(seedState)) / UINT_MAX_FLOAT;
}

void _spawnParticle(
    inout vec4 color,
    inout vec3 velocity,
    inout vec3 position,
    inout float timeToLive,
    inout float initialTimeToLive,
    inout float size,
    inout uint metadata
) {
    uint seed = floatBitsToUint(u_time) + gl_VertexID;

    color = vec4(1.0);
    velocity = vec3(0.0);
    position = vec3(0.0);
    timeToLive = 0.0;
    initialTimeToLive = 0.0;
    size = 1.0;
    metadata = 0U;

    for(uint i = 0; i < u_moduleCount; i++) {
        ParticleModule module = u_modules[i];
        if((module.flags & SPAWNLOCATIONMODULE_FLAG) != 0) {
            switch(module.type) {
                case PointSpawn:
                    position = vec3(0);
                    break;
                case BoxSpawn: {
                    position = vec3(mix(module.params[0].x, module.params[1].x, _rand01(seed)), mix(module.params[0].y, module.params[1].y, _rand01(seed)), mix(module.params[0].z, module.params[1].z, _rand01(seed)));
                    break;
                }
                case SphereSpawn: {
                    vec3 randomDir = normalize(vec3(_rand01(seed) * 2.0 - 1.0, _rand01(seed) * 2.0 - 1.0, _rand01(seed) * 2.0 - 1.0));
                    position = randomDir * mix(module.params[1].x, module.params[0].x, _rand01(seed));
                    break;
                }
                case ConeSpawn: {
                    float height = module.params[0].x;
                    float radius = module.params[1].x;

                // Generate random height along the cone
                    float randHeight = _rand01(seed) * height;
                    float localRadius = (randHeight / height) * radius;

                // Random angle around the cone axis
                    float theta = _rand01(seed) * 6.2831853; // 2π

                // Sample point on the circle at current height
                    float r = _rand01(seed) * localRadius;

                    position = vec3(r * cos(theta), randHeight, r * sin(theta));
                    break;
                }
                case DiskSpawn: {
                    vec3 randomDir = normalize(vec3(_rand01(seed) * 2.0 - 1.0, 0.0, _rand01(seed) * 2.0 - 1.0));
                    position = randomDir * mix(module.params[1].x, module.params[0].x, _rand01(seed));
                    break;
                }
                case LineSpawn: {
                    position = vec3(mix(module.params[0], module.params[1], _rand01(seed)));
                    break;
                }
            }
        } else if((module.flags & INITMODULE_FLAG) != 0) {
            switch(module.type) {
                case InitialVelocity: {
                    velocity = vec3(mix(module.params[0].x, module.params[1].x, _rand01(seed)), mix(module.params[0].y, module.params[1].y, _rand01(seed)), mix(module.params[0].z, module.params[1].z, _rand01(seed)));
                    break;
                }
                case InitialVelocityInCone: {
                    vec3 axis = module.params[2].xyz;

                    float outerAngle = module.params[3].x;
                    float innerAngle = module.params[4].x;

                // Generate a random angle between inner and outer cone
                    float angle = mix(innerAngle, outerAngle, _rand01(seed));

                // Sample random direction inside cone (spherical cap)
                    float z = cos(radians(angle)); // direction along axis
                    float sinTheta = sqrt(1.0 - z * z);

                    float phi = _rand01(seed) * 6.2831853; // random azimuth
                    float x = cos(phi) * sinTheta;
                    float y = sin(phi) * sinTheta;

                // Create direction in cone space (aligned to +Z)
                    vec3 dir = vec3(x, y, z);

                // Rotate to align with the actual axis direction
                    vec3 up = vec3(0.0, 0.0, 1.0);
                    vec3 rotationAxis = cross(up, axis);
                    float rotationAngle = acos(clamp(dot(up, axis), -1.0, 1.0));

                // Apply rotation if needed
                    if(length(rotationAxis) > 0.0001) {
                        rotationAxis = normalize(rotationAxis);
                        float cosA = cos(rotationAngle);
                        float sinA = sin(rotationAngle);

                    // Rodrigues' rotation formula
                        dir = dir * cosA +
                            cross(rotationAxis, dir) * sinA +
                            rotationAxis * dot(rotationAxis, dir) * (1.0 - cosA);
                    } else if(dot(up, axis) < 0.0) {
                    // Special case: axis is opposite of up
                        dir = -dir;
                    }

                    velocity = dir * mix(module.params[0].x, module.params[1].x, _rand01(seed));
                    break;
                }
                case InitialLifetime: {
                    timeToLive = mix(module.params[0].x, module.params[1].x, _rand01(seed));
                    initialTimeToLive = timeToLive;
                    break;
                }
                case InitialSize: {
                    size = mix(module.params[0].x, module.params[1].x, _rand01(seed));
                    break;
                }
                case InitialColor: {
                    color.rgb = vec3(mix(module.params[0].x, module.params[1].x, _rand01(seed)), mix(module.params[0].y, module.params[1].y, _rand01(seed)), mix(module.params[0].z, module.params[1].z, _rand01(seed)));
                    break;
                }
                case InitialAlpha: {
                    color.a = module.params[0].x;
                    break;
                }
                case InitialTexture: {
                    SET_BITS(metadata, module.metadata1, TEXINDEX_BITMASK, TEXINDEX_OFFSET);
                    break;
                }
            }
        }
    }
}

void _updateParticle(
    inout vec4 color,
    inout vec3 velocity,
    inout vec3 position,
    inout float timeToLive,
    inout float initialTimeToLive,
    inout float size,
    inout uint metadata
) {
    uint seed = floatBitsToUint(u_time) + gl_VertexID;

    position = position + velocity * u_deltaTime;
    timeToLive = timeToLive - u_deltaTime;

    if(timeToLive <= 0) {
        return;
    }

    float dragCoefficient = 0.0;
    vec3 jitterVel = vec3(0.0);

    for(uint i = 0; i < u_moduleCount; i++) {
        ParticleModule module = u_modules[i];
        if((module.flags & UPDATEMODULE_FLAG) != 0) {
            switch(module.type) {
                case Drag: {
                    dragCoefficient = module.params[0].x;
                    break;
                }
                case Acceleration: {
                    velocity = velocity + module.params[0].xyz * u_deltaTime;
                    break;
                }
                case Turbulence: {
                    jitterVel = normalize(vec3(_rand01(seed) * 2.0 - 1.0, _rand01(seed) * 2.0 - 1.0, _rand01(seed) * 2.0 - 1.0)) * module.params[0].x * u_deltaTime;
                    break;
                }
                case SizeOverLife: {
                    uint numKeyframes = module.metadata1;
                    if(numKeyframes > 0) {
                        float t = clamp(1.0 - timeToLive / initialTimeToLive, 0.0, 1.0);

                    // Extract the first and last time values
                        float t_first = module.params[0][0];
                        float t_last = module.params[0][numKeyframes - 1];

                        if(t <= t_first) {
                            size = module.params[1].x; // Snap to first value
                        } else if(t >= t_last) {
                            size = module.params[1].x; // Snap to last value
                        } else {
                            for(uint i = 0; i < numKeyframes - 1; i++) {
                                float t0 = module.params[0][i];
                                float t1 = module.params[0][i + 1];

                                if(t >= t0 && t <= t1) {
                                    float s0 = module.params[i + 1].x;
                                    float s1 = module.params[i + 2].x;

                                    float localT = (t - t0) / max(t1 - t0, 1e-5);
                                    size = mix(s0, s1, localT);
                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
                case ColorOverLife: {
                    uint numKeyframes = module.metadata1;
                    if(numKeyframes > 0) {
                        float t = clamp(1.0 - timeToLive / initialTimeToLive, 0.0, 1.0);

                    // Extract the first and last time values
                        float t_first = module.params[0][0];
                        float t_last = module.params[0][numKeyframes - 1];

                        if(t <= t_first) {
                            color.rgb = module.params[1].rgb; // Snap to first value
                        } else if(t >= t_last) {
                            color.rgb = module.params[1].rgb; // Snap to last value
                        } else {
                        // Interpolate
                            for(uint i = 0; i < numKeyframes - 1; i++) {
                                float t0 = module.params[0][i];
                                float t1 = module.params[0][i + 1];

                                if(t >= t0 && t <= t1) {
                                    vec3 c0 = module.params[i + 1].rgb;
                                    vec3 c1 = module.params[i + 2].rgb;

                                    float localT = (t - t0) / max(t1 - t0, 1e-5);
                                    color.rgb = mix(c0, c1, localT);
                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
                case AlphaOverLife: {
                    uint numKeyframes = module.metadata1;
                    if(numKeyframes > 0) {
                        float t = clamp(1.0 - timeToLive / initialTimeToLive, 0.0, 1.0);

                    // Extract the first and last time values
                        float t_first = module.params[0][0];
                        float t_last = module.params[0][numKeyframes - 1];

                        if(t <= t_first) {
                            color.a = module.params[1].x; // Snap to first value
                        } else if(t >= t_last) {
                            color.a = module.params[1].x; // Snap to last value
                        } else {
                            for(uint i = 0; i < numKeyframes - 1; i++) {
                                float t0 = module.params[0][i];
                                float t1 = module.params[0][i + 1];

                                if(t >= t0 && t <= t1) {
                                    float a0 = module.params[i + 1].x;
                                    float a1 = module.params[i + 2].x;

                                    float localT = (t - t0) / max(t1 - t0, 1e-5);
                                    color.a = mix(a0, a1, localT);
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
                    if(offsetScale > 0.0) {
                        uint startSeed = gl_VertexID;
                        offset = _rand01(startSeed) * offsetScale * (float(numFrames) / fps);
                    }
                    float age = 1.0 - (timeToLive / (initialTimeToLive + 0.0001));
                    float elapsedRelativeTime = age * initialTimeToLive;
                    uint frameIndex = uint(floor((elapsedRelativeTime + offset) * fps)) % numFrames;
                    SET_BITS(metadata, baseTexIndex + frameIndex, TEXINDEX_BITMASK, TEXINDEX_OFFSET);
                    break;
                }
            }
        }
    }

    // Apply drag
    velocity *= max(1.0 - dragCoefficient * u_deltaTime, 0.0);
    // Apply turbulence
    velocity += jitterVel;
}

void processParticles(
    inout vec4 color,
    inout vec3 velocity,
    inout vec3 position,
    inout float timeToLive,
    inout float initialTimeToLive,
    inout float size,
    inout uint metadata
) {
    if(timeToLive <= 0.0) {
        if((u_flags & DYNAMIC_SPAWNRATE) != 0) {
            if(u_spawnCount == 0) {
                return;
            }

            // Spawn based on free ringbuffer slots
            uint begin = u_particleSpawnOffset;
            uint end = (u_particleSpawnOffset + u_spawnCount - 1) % u_allocatedParticleCount;

            if(begin <= end && (gl_VertexID >= begin && gl_VertexID <= end)) {
                // Normal case, no wrap
                _spawnParticle(color, velocity, position, timeToLive, initialTimeToLive, size, metadata);
            } else if(begin > end && (gl_VertexID >= begin || gl_VertexID <= end)) {
                // Wrapped case
                _spawnParticle(color, velocity, position, timeToLive, initialTimeToLive, size, metadata);
            }
        } else {
            // Force respawn all
            _spawnParticle(color, velocity, position, timeToLive, initialTimeToLive, size, metadata);
        }
    } else {
        _updateParticle(color, velocity, position, timeToLive, initialTimeToLive, size, metadata);
    }
}
