#include "World.h"

#include <json/JsonParser.h>

#include <algorithm>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "Application.h"
#include "Logger.h"
#include "engine/assets/AssetManager.h"
#include "engine/assets/EngineAssets.h"
#include "engine/assets/meshcreate/ChunkMeshCreate.h"
#include "engine/rendering/Renderer.h"
#include "engine/rendering/material/ChunkMaterial.h"
#include "engine/scene/renderables/StaticMesh.h"
#include "foundation/threading/ThreadPool.h"
#include "foundation/util/Utility.h"
#include "game/world/generation/ChunkMeshGeneration.h"
#include "game/world/generation/PerlinNoise.h"

static void generateChunkBlocks(Block* blocks, const glm::ivec3& chunkPos, uint32_t seed) {
    PerlinNoise noiseGenerator(seed);

    // Generate height values for the xz plane in global coordinates
    std::unique_ptr<float[]> heightValues = noiseGenerator.generatePerlinNoise(
        {CHUNK_WIDTH, CHUNK_DEPTH}, {chunkPos.x, chunkPos.z}, 32, 2
    );
    std::unique_ptr<float[]> ironOre = noiseGenerator.generatePerlinNoise(
        {CHUNK_WIDTH, CHUNK_HEIGHT, CHUNK_DEPTH}, {chunkPos.x, chunkPos.y, chunkPos.z}, 16, 2
    );

    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_DEPTH; z++) {
                // Surface height
                float surfaceHeight = heightValues[z * CHUNK_DEPTH + x] * 10.0f;

                // Global y coordinate
                int globalY = chunkPos.y + y;

                // Get iron ore noise value
                float ironValue = ironOre[z * CHUNK_HEIGHT * CHUNK_WIDTH + y * CHUNK_WIDTH + x];

                // Conditions for placing blocks
                if (globalY < static_cast<int>(floor(surfaceHeight))) {
                    // Default to stone
                    blocks[chunkBlockIndex(x, y, z)] = {STONE, true};

                    // Apply ore generation logic
                    if (globalY < 0) {           // Only generate iron below Y=60
                        float threshold = 0.6f;  // Adjust spawn probability

                        if (ironValue > threshold) {
                            blocks[chunkBlockIndex(x, y, z)] = {IRON_ORE, true};
                        }
                    }
                } else if (globalY == static_cast<int>(floor(surfaceHeight))) {
                    blocks[chunkBlockIndex(x, y, z)] = {GRASS, true};
                } else {
                    blocks[chunkBlockIndex(x, y, z)] = {AIR, false};
                }
            }
        }
    }
}

std::unordered_set<glm::ivec3, coord_hash> World::determineActiveChunks(const glm::ivec3& position) {
    std::unordered_set<glm::ivec3, coord_hash> activeChunks;
    glm::ivec3 centerChunk(
        (position.x < 0 ? (position.x - CHUNK_WIDTH + 1) : position.x) / CHUNK_WIDTH * CHUNK_WIDTH,
        (position.y < 0 ? (position.y - CHUNK_HEIGHT + 1) : position.y) / CHUNK_HEIGHT * CHUNK_HEIGHT,
        (position.z < 0 ? (position.z - CHUNK_DEPTH + 1) : position.z) / CHUNK_DEPTH * CHUNK_DEPTH
    );

    // Compute the range of chunk coordinates to load
    for (int x = -chunkLoadingDistance * CHUNK_WIDTH; x <= chunkLoadingDistance * CHUNK_WIDTH; x += CHUNK_WIDTH) {
        for (int y = -chunkLoadingDistance * CHUNK_HEIGHT; y <= chunkLoadingDistance * CHUNK_HEIGHT;
             y += CHUNK_HEIGHT) {
            for (int z = -chunkLoadingDistance * CHUNK_DEPTH; z <= chunkLoadingDistance * CHUNK_DEPTH;
                 z += CHUNK_DEPTH) {
                glm::ivec3 offset(x * CHUNK_WIDTH, y * CHUNK_HEIGHT, z * CHUNK_DEPTH);
                glm::ivec3 chunkPos = centerChunk + glm::ivec3(x, y, z);

                // Only include chunks within Euclidean distance
                if (glm::length(glm::vec3(chunkPos - centerChunk)) <= chunkLoadingDistance * CHUNK_WIDTH) {
                    activeChunks.insert(chunkPos);
                }
            }
        }
    }
    return activeChunks;
}

World::World(const std::filesystem::path& worldDir) : m_worldDir(worldDir), m_cStorage(worldDir) {
    m_taskContext = Application::getContext()->workerPool->getNewTaskContext();

    // Load world data
    Json::JsonValue info = Json::parseJson(readFile(worldDir / "info.json"));
    m_seed = static_cast<uint32_t>(std::stoul(info["seed"].toString()));

    AssetManager* assets = Application::getContext()->assets;
    Future<Shader> mainShader = assets->request<Shader>(Assets::Shader::CHUNK);
    Future<Shader> depthShader = assets->request<Shader>(Assets::Shader::CHUNK_DEPTH);
    Future<Shader> ssaoGBuffShader = assets->request<Shader>(Assets::Shader::CHUNK_SSAO_GBUFFER);
    Future<Texture> texture = assets->request<Texture>(Assets::Texture::BLOCK_TEX_ATLAS);
    m_chunkMaterial = std::make_shared<ChunkMaterial>(mainShader, depthShader, ssaoGBuffShader, texture);
}

World::~World() {
    ThreadPool* pool = Application::getContext()->workerPool;
    pool->destroyTaskContext(m_taskContext);
    pool->waitForCurrentActiveTasks();
}

Chunk* World::getChunk(const glm::ivec3& location) {
    auto it = m_loadedChunks.find(location);
    if (it != m_loadedChunks.end()) {
        if (it->second.isLoaded()) {
            return &it->second;
        }
    }
    return nullptr;
}

void World::updateChunks(const glm::ivec3& position) {
    // Step 1: Determine active chunk positions
    std::unordered_set<glm::ivec3, coord_hash> activeChunks = determineActiveChunks(position);

    // Step 2: Unload chunks that are no longer in the active set
    for (auto it = m_loadedChunks.begin(); it != m_loadedChunks.end();) {
        if (activeChunks.find(it->first) == activeChunks.end()) {
            if (it->second.isMarkedForSave()) {
                // Save chunk that will be unloaded but has changes
                glm::ivec3 chunkPos = it->first;
                std::shared_ptr<Block[]> blockData = std::move(it->second.m_blocks.value());

                Future<void> future(
                    [this, chunkPos, blockData]() {
                        try {
                            m_cStorage.saveChunkData(chunkPos, blockData.get());
                        } catch (const std::exception& e) {
                            lgr::lout.error(e.what());
                        }
                    },
                    m_taskContext
                );
                future.start();
            }
            m_scene.destroy(it->second.m_mesh);
            it = m_loadedChunks.erase(it);
        } else {
            ++it;  // Chunk is still active
        }
    }

    // Step 3: Process optional pending mesh build
    for (auto it = m_loadedChunks.begin(); it != m_loadedChunks.end(); it++) {
        it->second.tryCommitRebuild();
    }

    // Step 4: TODO Process pending changes for unloaded chunks
    // * Currently unhanlded *
    m_pendingChanges.clear();

    // Step 5: Load chunks that are in the active set but not yet loaded or need mesh rebuild
    for (const glm::ivec3& chunkPos : activeChunks) {
        auto it = m_loadedChunks.find(chunkPos);
        if (it == m_loadedChunks.end()) {
            // Chunk does not exist -> Needs to be fully loaded

            // Create new one
            Future<std::unique_ptr<Block[]>> blockGenFuture(
                [this, chunkPos]() {
                    std::unique_ptr<Block[]> blocks;
                    if (m_cStorage.hasChunk(chunkPos)) {
                        blocks = m_cStorage.loadChunkData(chunkPos);
                    } else {
                        blocks = std::unique_ptr<Block[]>(new Block[BLOCKS_PER_CHUNK], std::default_delete<Block[]>());
                        generateChunkBlocks(blocks.get(), chunkPos, m_seed);
                    }

                    return std::move(blocks);
                },
                m_taskContext
            );
            blockGenFuture.start();

            Future<CPURenderData<CompactChunkVertex>> cpuMeshBuildFuture(
                [this, blockGenFuture]() { return generateMeshForChunkGreedy(blockGenFuture.value().get(), texMap); },
                m_taskContext
            );
            cpuMeshBuildFuture.dependsOn(blockGenFuture).start();

            Future<StaticMesh::Asset> meshCreateFuture(
                [cpuMeshBuildFuture]() { return createStaticMeshAsset(cpuMeshBuildFuture.value()); },
                m_taskContext,
                Executor::Main
            );
            meshCreateFuture.dependsOn(cpuMeshBuildFuture).start();

            // Put placeholder chunk (Chunk with no block data / mesh)
            Chunk placeHolder = Chunk();
            placeHolder.m_blocks = blockGenFuture;
            placeHolder.m_mesh = m_scene.create<StaticMesh>(meshCreateFuture, m_chunkMaterial);
            placeHolder.m_mesh->getLocalTransform().setPosition(chunkPos);
            m_loadedChunks[chunkPos] = std::move(placeHolder);

        } else if (it->second.isChanged() && !it->second.isBeingRebuild()) {
            // Chunk already exists and needs a rebuild (and no other worker is currently rebuilding this) -> rebuild
            // only mesh data

            // Make copy of blockdata
            std::shared_ptr<Block[]> blocksCopy(new Block[BLOCKS_PER_CHUNK], std::default_delete<Block[]>());
            const Block* src = it->second.blocks();
            std::copy(src, src + BLOCKS_PER_CHUNK, blocksCopy.get());

            it->second.m_changed = false;

            Future<CPURenderData<CompactChunkVertex>> cpuMeshBuildFuture(
                [this, blocksCopy]() { return generateMeshForChunkGreedy(blocksCopy.get(), texMap); }, m_taskContext
            );
            cpuMeshBuildFuture.start();

            Future<StaticMesh::Asset> meshCreateFuture(
                [cpuMeshBuildFuture]() { return createStaticMeshAsset(cpuMeshBuildFuture.value()); },
                m_taskContext,
                Executor::Main
            );
            meshCreateFuture.dependsOn(cpuMeshBuildFuture).start();

            m_loadedChunks[chunkPos].m_pendingRebuildMesh = meshCreateFuture;
        }
    }
}

void World::syncedSaveChunks() {
    for (auto& entry : m_loadedChunks) {
        if (entry.second.isMarkedForSave()) {
            m_cStorage.saveChunkData(entry.first, entry.second.blocks());
            entry.second.m_isMarkedForSave = false;
        }
    }
}

void World::setBlock(const glm::ivec3& position, uint16_t newBlock) {
    glm::ivec3 chunkPos = Chunk::worldToChunkOrigin(position);
    if (Chunk* chunk = getChunk(chunkPos)) {
        // Immediate data change if chunk is loaded
        glm::ivec3 relChunkPos = Chunk::worldToChunkLocal(chunkPos, position);
        chunk->m_blocks.value()[chunkBlockIndex(relChunkPos.x, relChunkPos.y, relChunkPos.z)] = {
            newBlock, newBlock != AIR
        };
        chunk->m_changed = true;
        chunk->m_isMarkedForSave = true;
    } else {
        // Queue changes
        m_pendingChanges[position] = newBlock;
    }
}

void World::update(float deltaTime) { m_scene.update(deltaTime); }
