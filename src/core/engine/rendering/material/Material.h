#ifndef TOOMANYBLOCKS_MATERIAL_H
#define TOOMANYBLOCKS_MATERIAL_H

struct RenderContext;

enum PassType {
    TransformFeedback,
    ShadowPass,
    AmbientOcclusion,
    OpaquePass,
    TransparencyPass
};

class Material {
public:
    virtual ~Material() = default;

    virtual bool isReady() const { return false; }
    virtual bool supportsPass(PassType passType) const = 0;
    virtual void bindForPass(PassType passType, const RenderContext& context) = 0;
    virtual void bindForObjectDraw(PassType passType, const RenderContext& context) = 0;
};

#endif
