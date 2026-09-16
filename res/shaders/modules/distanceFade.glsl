float calculateDistanceFade(vec3 cameraPosition, vec3 worldPosition, float startDistance,float fadeDistance) {
    float distanceToCamera = length(cameraPosition - worldPosition);
    if(distanceToCamera <= startDistance)
        return 1.0;

    return 1.0 - smoothstep(startDistance, startDistance + fadeDistance, distanceToCamera);
}