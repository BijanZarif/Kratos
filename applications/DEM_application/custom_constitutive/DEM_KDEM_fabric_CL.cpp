// System includes
#include <string>
#include <iostream>
#include <cmath>

// Project includes
#include "DEM_application.h"
#include "DEM_KDEM_fabric_CL.h"
#include "custom_elements/spheric_particle.h"

namespace Kratos {


    DEMContinuumConstitutiveLaw::Pointer DEM_KDEMFabric::Clone() const {
        DEMContinuumConstitutiveLaw::Pointer p_clone(new DEM_KDEMFabric(*this));
        return p_clone;
    }

    void DEM_KDEMFabric::SetConstitutiveLawInProperties(Properties::Pointer pProp) const {
        std::cout << "\nAssigning DEM_KDEMFabric to properties " << pProp->Id() << std::endl;
        pProp->SetValue(DEM_CONTINUUM_CONSTITUTIVE_LAW_POINTER, this->Clone());
    }    
                                  
    
    void DEM_KDEMFabric::ComputeParticleRotationalMoments(SphericContinuumParticle* element,
                                                    SphericContinuumParticle* neighbor,
                                                    double equiv_young,
                                                    double distance,
                                                    double calculation_area,
                                                    double LocalCoordSystem[3][3],
                                                    array_1d<double, 3>& mContactMoment) {

        KRATOS_TRY
        double LocalRotationalMoment[3]      = {0.0};
        double LocalDeltaRotatedAngle[3]     = {0.0};
        double LocalDeltaAngularVelocity[3]  = {0.0};
        
        array_1d<double, 3> GlobalDeltaRotatedAngle;
        noalias(GlobalDeltaRotatedAngle) = element->GetGeometry()[0].FastGetSolutionStepValue(PARTICLE_ROTATION_ANGLE) - neighbor->GetGeometry()[0].FastGetSolutionStepValue(PARTICLE_ROTATION_ANGLE);
        array_1d<double, 3> GlobalDeltaAngularVelocity;
        noalias(GlobalDeltaAngularVelocity) = element->GetGeometry()[0].FastGetSolutionStepValue(ANGULAR_VELOCITY) - neighbor->GetGeometry()[0].FastGetSolutionStepValue(ANGULAR_VELOCITY);

        GeometryFunctions::VectorGlobal2Local(LocalCoordSystem, GlobalDeltaRotatedAngle, LocalDeltaRotatedAngle);
        GeometryFunctions::VectorGlobal2Local(LocalCoordSystem, GlobalDeltaAngularVelocity, LocalDeltaAngularVelocity);
        GeometryFunctions::VectorGlobal2Local(LocalCoordSystem, mContactMoment, LocalRotationalMoment);
        
        const double equivalent_radius = sqrt(calculation_area / KRATOS_M_PI);
        const double Inertia_I = 0.25 * KRATOS_M_PI * equivalent_radius * equivalent_radius * equivalent_radius * equivalent_radius;
        const double Inertia_J = 2.0 * Inertia_I; // This is the polar inertia
        const double rot_k = 0.1; // TODO: Hardcoded for the time being. Do this properly, the idea is to have a very small flection resistance
                        
        const double element_mass  = element->GetMass();
        const double neighbor_mass = neighbor->GetMass();
        const double equiv_mass    = element_mass * neighbor_mass / (element_mass + neighbor_mass);
        
        // Viscous parameter taken from J.S.Marshall, 'Discrete-element modeling of particle aerosol flows', section 4.3. Twisting resistance
        const double alpha = 0.9; // Hardcoded only for testing purposes. This value depends on the restitution coefficient and goes from 0.1 to 1.0
        const double visc_param = 0.5 * equivalent_radius * equivalent_radius * alpha * sqrt(1.33333333333333333 * equiv_mass * equiv_young * equivalent_radius);
             
        //equiv_young or G in torsor (LocalRotationalMoment[2]) ///////// TODO
        LocalRotationalMoment[0] -= (rot_k * equiv_young * Inertia_I * LocalDeltaRotatedAngle[0] / distance + visc_param * LocalDeltaAngularVelocity[0]);
        LocalRotationalMoment[1] -= (rot_k * equiv_young * Inertia_I * LocalDeltaRotatedAngle[1] / distance + visc_param * LocalDeltaAngularVelocity[1]);
        LocalRotationalMoment[2] -= (rot_k * equiv_young * Inertia_J * LocalDeltaRotatedAngle[2] / distance + visc_param * LocalDeltaAngularVelocity[2]);
        
        // Judge if the rotation spring is broken or not
        /*
        double ForceN  = LocalElasticContactForce[2];
        double ForceS  = sqrt(LocalElasticContactForce[0] * LocalElasticContactForce[0] + LocalElasticContactForce[1] * LocalElasticContactForce[1]);
        double MomentS = sqrt(LocalRotaSpringMoment[0] * LocalRotaSpringMoment[0] + LocalRotaSpringMoment[1] * LocalRotaSpringMoment[1]);
        double MomentN = LocalRotaSpringMoment[2];
        // bending stress and axial stress add together, use edge of the bar will failure first
        double TensiMax = -ForceN / calculation_area + MomentS        / Inertia_I * equiv_radius;
        double ShearMax =  ForceS  / calculation_area + fabs(MomentN)  / Inertia_J * equiv_radius;
        if (TensiMax > equiv_tension || ShearMax > equiv_cohesion) {
            mRotaSpringFailureType[i_neighbor_count] = 1;
            LocalRotaSpringMoment[0] = LocalRotaSpringMoment[1] = LocalRotaSpringMoment[2] = 0.0;
            //LocalRotaSpringMoment[1] = 0.0;
            //LocalRotaSpringMoment[2] = 0.0;
        }
        */
        GeometryFunctions::VectorLocal2Global(LocalCoordSystem, LocalRotationalMoment, mContactMoment);
        KRATOS_CATCH("")
    }//ComputeParticleRotationalMoments
    
    void DEM_KDEMFabric::AddPoissonContribution(const double equiv_poisson, double LocalCoordSystem[3][3], double& normal_force, 
                                          double calculation_area, Matrix* mSymmStressTensor, SphericParticle* element1, SphericParticle* element2) {
        KRATOS_TRY         
        KRATOS_CATCH("")        
    } //AddPoissonContribution

} // namespace Kratos