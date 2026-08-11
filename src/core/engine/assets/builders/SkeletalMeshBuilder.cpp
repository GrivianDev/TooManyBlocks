#include "SkeletalMeshBuilder.h"

#include "engine/blueprints/SkeletalMeshBlueprint.h"

Future<SkeletalMesh::Asset> build(const Future<CPUSkeletalMeshData>& cpuMesh) {
    Future<SkeletalMesh::Asset> sharedMeshData(
        [cpuMesh]() { return createSkeletalMeshAsset(cpuMesh.value()); }, DEFAULT_TASKCONTEXT, Executor::Main
    );
    return sharedMeshData.dependsOn(cpuMesh).start();
}