#include "AnimationStateMachine.h"

#include <stdexcept>

AnimationStateMachine::State* AnimationStateMachine::findState(const std::string& name) {
    for (State& s : m_states) {
        if (s.name == name) return &s;
    }
    return nullptr;
}

AnimationStateMachine::AnimationStateMachine(SkeletalMesh* owner)
    : m_owner(owner),
      m_currentState(nullptr),
      m_previousState(nullptr),
      m_transitionTime(0.0f),
      m_transitionDuration(0.0f) {}

void AnimationStateMachine::addState(const std::string& name, std::unique_ptr<AnimationNode> node) {
    m_states.emplace_back(State{name, std::move(node)});
}

void AnimationStateMachine::addTransition(
    const std::string& from,
    const std::string& to,
    std::function<bool(const SkeletalMesh*)> condition,
    float duration
) {
    State* state = findState(from);
    if (!state) throw std::runtime_error("Can not add transition to non existent state");
    state->transitions.emplace_back(Transition{to, std::move(condition), duration});
}

void AnimationStateMachine::transitionTo(const std::string& name, float duration) {
    State* nextState = findState(name);
    if (!nextState || nextState == m_currentState) return;

    m_transitionTime = 0.0f;
    m_transitionDuration = std::max(0.0f, duration);

    m_previousState = m_transitionDuration > 0.0f ? m_currentState : nullptr;
    m_currentState = nextState;
}

void AnimationStateMachine::update(float deltaTime) {
    if (!m_currentState) return;

    if (m_previousState) {
        if (m_previousState->node) m_previousState->node->update(deltaTime);
        if (m_currentState->node) m_currentState->node->update(deltaTime);

        m_transitionTime += deltaTime;

        if (m_transitionTime >= m_transitionDuration) {
            m_transitionTime = m_transitionDuration;
            // Transition is complete.
            m_previousState = nullptr;
        }

        return;
    }

    if (m_currentState->node) m_currentState->node->update(deltaTime);

    for (const Transition& transition : m_currentState->transitions) {
        if (transition.condition(m_owner)) {
            transitionTo(transition.targetState, transition.duration);
            break;
        }
    }
}

void AnimationStateMachine::evaluate(std::vector<Transform>& outPose) {
    if (!m_currentState || !m_currentState->node) return;

    // no transition case
    if (!m_previousState) {
        m_currentState->node->evaluate(outPose);
        return;
    }

    m_previousPose.resize(outPose.size());
    m_currentPose.resize(outPose.size());

    m_previousState->node->evaluate(m_previousPose);
    m_currentState->node->evaluate(m_currentPose);

    float alpha = 1.0f;
    if (m_transitionDuration > 0.0f) {
        alpha = m_transitionTime / m_transitionDuration;
        alpha = std::clamp(alpha, 0.0f, 1.0f);
    }

    for (size_t i = 0; i < outPose.size(); i++) {
        outPose[i] = m_previousPose[i].interpolate(m_currentPose[i], alpha);
    }
}
