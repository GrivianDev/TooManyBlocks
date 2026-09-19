#include "StaticMesh.h"

#include <GL/glew.h>

#include "foundation/threading/Future.h"

StaticMesh::StaticMesh(const Future<Asset>& asset, std::shared_ptr<Material> material)
    : Renderable(material), m_asset(asset) {
    // TODO this breaks once the component is evicted before the boundsUpdate ran, it must be cancelled or smth.
    // This also probably affects the SkeletalMesh.
    Future<void> boundsUpdate = Future<void>(
        [this, asset]() { setLocalBounds(asset.value().bounds); }, DEFAULT_TASKCONTEXT, Executor::Main
    );

    boundsUpdate.dependsOn(asset).start();
}

void StaticMesh::draw() const {
    if (m_asset.isReady()) {
        m_asset.value().renderData->drawAs(GL_TRIANGLES);
    }
}
