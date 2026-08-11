#include "StaticMeshBuilder.h"

#include "engine/blueprints/ChunkMeshBlueprint.h"
#include "engine/blueprints/StaticMeshBlueprint.h"

Future<StaticMesh::Asset> build(const Future<CPURenderData<Vertex>>& cpuMesh) {
    Future<StaticMesh::Asset> meshSharedData(
        [cpuMesh]() { return createStaticMeshAsset(cpuMesh.value()); }, DEFAULT_TASKCONTEXT, Executor::Main
    );
    return meshSharedData.dependsOn(cpuMesh).start();
}

Future<StaticMesh::Asset> build(const Future<CPURenderData<CompactChunkVertex>>& cpuMesh) {
    Future<StaticMesh::Asset> meshSharedData(
        [cpuMesh]() { return createStaticMeshAsset(cpuMesh.value()); }, DEFAULT_TASKCONTEXT, Executor::Main
    );
    return meshSharedData.dependsOn(cpuMesh).start();
}
