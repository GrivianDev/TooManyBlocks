const float _UINT_MAX_FLOAT = 4294967295.0;

uint pcg_hash(uint seedState) {
    uint state = seedState * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float rand01(inout uint seedState) {
    seedState++; // Modify passed seed to ensure random numbers on multiple calls
    return float(pcg_hash(seedState)) / _UINT_MAX_FLOAT;
}