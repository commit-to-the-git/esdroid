#include "../include/particle.h"

#include "../include/particle_system.h"

dphysics::Particle::Particle() : ysObject("PARTICLE") {
    m_life = 0.0f;
    m_age = 0.0f;
    m_scale = 1.0f;
    m_damping = 1.0f;
    m_expansionRate = 0.0f;
    m_density = 1.0f;

    m_parent = NULL;
}

dphysics::Particle::~Particle() {
    /* void */
}

/*
void dbasic::particle::renderdeltaengine *engine int layer {
    ysvector finalposition = ysmath::addm_position m_parent->getposition

    ysmatrix scale = ysmath::scaletransformysmath::loadscalarm_scale
    ysmatrix pos = ysmath::translationtransformfinalposition
    ysmatrix final = ysmath::matmultscale pos

    float inv_density = 1.0f - m_density
    engine->setmultiplycolorysvector40.5f * inv_density 0.5f * inv_density 0.5f * inv_density 1.0f - m_age / m_life * m_density
    engine->setobjecttransformfinal
    engine->drawimagem_texture layer
    engine->resetmultiplycolor
}
*/

void dphysics::Particle::Update(float timePassed) {
    m_velocity = ysMath::Mul(m_velocity, ysMath::LoadScalar(m_damping));
    m_position = ysMath::Add(m_position, ysMath::Mul(m_velocity, ysMath::LoadScalar(timePassed)));

    m_scale += m_expansionRate * timePassed;
    m_age += timePassed;
}
