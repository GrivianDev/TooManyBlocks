#ifndef TOOMANYBLOCKS_ANIMATIONSTATEMACHINE_H
#define TOOMANYBLOCKS_ANIMATIONSTATEMACHINE_H

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "engine/scene/Transform.h"
#include "engine/animation/AnimationNode.h"
#include "engine/scene/renderables/SkeletalMesh.h"

class AnimationStateMachine : public AnimationNode {
public:
    struct Transition {
        std::string targetState;
        std::function<bool(const SkeletalMesh*)> condition;
        float duration;
    };

    struct State {
        std::string name;
        std::unique_ptr<AnimationNode> node;
        std::vector<Transition> transitions;
    };

private:
    SkeletalMesh* m_owner;

    std::vector<State> m_states;

    State* m_currentState;
    State* m_previousState;

    std::vector<Transform> m_previousPose;
    std::vector<Transform> m_currentPose;

    float m_transitionTime;
    float m_transitionDuration;

    State* findState(const std::string& name);

    void transitionTo(State* state, float duration);

public:
    AnimationStateMachine(SkeletalMesh* owner);
    virtual ~AnimationStateMachine() = default;

    void addState(const std::string& name, std::unique_ptr<AnimationNode> node);
    void addTransition(
        const std::string& from,
        const std::string& to,
        std::function<bool(const SkeletalMesh*)> condition,
        float duration = 0.0f
    );

    void transitionTo(const std::string& name, float duration = 0.0f);

    inline bool isTransitioning() const { return m_previousState != nullptr; };

    void update(float deltaTime) override;

    void evaluate(std::vector<Transform>& outPose) override;
};

#endif
