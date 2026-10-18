#ifndef ATG_ENGINE_SIM_VEHICLE_DRAG_CONSTRAINT_H
#define ATG_ENGINE_SIM_VEHICLE_DRAG_CONSTRAINT_H

#include "scs.h"

class Vehicle;
class VehicleDragConstraint : public atg_scs::Constraint {
public:
    VehicleDragConstraint();
    virtual ~VehicleDragConstraint();

    void initialize(atg_scs::RigidBody *rotatingMass, Vehicle *vehicle);

    virtual void calculate(Output *output, atg_scs::SystemState *system);

    // the touch brake button drives this 0 to 1
    void setBrake(double brake) { m_brake = brake; }

    double m_ks;
    double m_kd;

private:
    Vehicle *m_vehicle;
    double m_brake;
};

#endif /* ATG_ENGINE_SIM_VEHICLE_DRAG_CONSTRAINT_H  */
