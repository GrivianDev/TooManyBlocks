#include "StaticMesh.h"

#include <GL/glew.h>

void StaticMesh::draw() const {
    if (m_asset.isReady()) {
        m_asset.value().renderData->drawAs(GL_TRIANGLES);
    }
}

BoundingBox StaticMesh::getBoundingBox() const {
    if (m_asset.isReady()) {
        return m_asset.value().bounds;
    }
    return Renderable::getBoundingBox();
}