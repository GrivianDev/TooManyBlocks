void discardIfDead(float timeToLive) {
    if(timeToLive <= 0.0) {
        discard; // Discard pixels that belong to dead particles
    }
}