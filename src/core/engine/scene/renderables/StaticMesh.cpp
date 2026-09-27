#include "StaticMesh.h"

#include <GL/glew.h>

#include "foundation/threading/Future.h"

StaticMesh::StaticMesh(const Future<Asset>& asset, std::shared_ptr<Material> material)
    : Renderable(material), m_asset(asset) {
    m_boundsUpdate = Future<void>(
        [this, asset]() { setLocalBounds(asset.value().bounds); }, DEFAULT_TASKCONTEXT, Executor::Main
    );

    m_boundsUpdate.dependsOn(asset).start();
}

StaticMesh::~StaticMesh() {
    m_boundsUpdate.cancel();
}

void StaticMesh::draw() const {
    if (m_asset.isReady()) {
        m_asset.value().renderData->drawAs(GL_TRIANGLES);
    }
}
