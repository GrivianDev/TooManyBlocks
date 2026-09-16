vec3 lightWithAmbient(vec3 baseColor, vec3 lightContrib, float ambientFactor) {
    float ambient = clamp(ambientFactor, 0.0, 1.0);
    return (ambient * baseColor) + ((1.0 - ambient) * clamp(lightContrib * baseColor, 0.0, 1.0));
}