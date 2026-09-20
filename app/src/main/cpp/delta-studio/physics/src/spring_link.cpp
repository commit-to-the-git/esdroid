#include "../include/spring_link.h"

#include "../include/rigid_body.h"

dphysics::SpringLink::SpringLink() {
    m_relativePos1 = ysMath::Constants::Zero;
    m_relativePos2 = ysMath::Constants::Zero;

    m_falloffPattern = FALLOFF_LINEAR;
    m_strength = 0.01f;
    m_length = 0.0f;
}

dphysics::SpringLink::~SpringLink() {
    /* void */
}

int dphysics::SpringLink::GenerateCollisions(Collision *collisionArray) {
    ysVector actualPosition1 = m_body1->Transform.LocalToWorldSpace(m_relativePos1);
    ysVector actualPosition2 = m_body2->Transform.LocalToWorldSpace(m_relativePos2);

    ysVector delta = ysMath::Sub(actualPosition2, actualPosition1);
    ysVector length = ysMath::Magnitude(delta);
    ysVector normal = ysMath::Div(delta, length);

    float penetration = ysMath::GetScalar(length) - m_length;
    float force = 0.0f;

    if (m_falloffPattern == FALLOFF_LINEAR)
        force = m_strength * penetration;
    else if (m_falloffPattern == FALLOFF_R2)
        force = m_strength * 1 / (penetration * abs(penetration));

    float max = 10.0f;
    if (force > max) force = max;
    else if (force < -max) force = -max;

    ysVector forceVec = ysMath::Mul(normal, ysMath::LoadScalar(force));
    ysMath::ExtendVector(forceVec);

    m_body1->AddForceLocalSpace(forceVec, m_relativePos1);
    // m_body2->addforcelocalspaceysmath::negate3forcevec m_relativepos2

    return 0;
}

/*
void dphysics::springlink::drawdebugdeltaengine *engine int layer {
    int red3 = { 255 0 0 }
    int blue3 = { 0 0 255 }

    ysvector actualposition1 = m_body1->getglobalspacem_relativepos1
    ysvector actualposition2 = m_body2->getglobalspacem_relativepos2

    engine->SetObjectTransform(ysMath::TranslationTransform(actualPosition2));
    engine->drawboxred 20 20 layer

    engine->SetObjectTransform(ysMath::TranslationTransform(actualPosition1));
    engine->drawboxblue 20 20 layer
}
*/
