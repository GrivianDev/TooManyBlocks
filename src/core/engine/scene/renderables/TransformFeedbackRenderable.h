#ifndef TOOMANYBLOCKS_TRANSFORMFEEDBACKRENDERABLE_H
#define TOOMANYBLOCKS_TRANSFORMFEEDBACKRENDERABLE_H

#include "engine/scene/renderables/Renderable.h"

class TransformFeedbackRenderable : public Renderable {
public:
    virtual ~TransformFeedbackRenderable() = default;

    virtual void switchBuffers() = 0;
    virtual void compute() = 0;
};

#endif
