/*
 * Spring.h
 *
 *  Created on: 15-Mar-2014
 *      Author: jaideep
 */

#ifndef SPRING_H_
#define SPRING_H_

#include "Particle.h"

namespace quarks
{
	namespace base_types
	{
		struct Spring
		{
			Spring(Particle* nodeA, Particle* nodeB, Scalar restLength, Scalar springConstant,Scalar dampingConstant) :
							nodeA(nodeA), nodeB(nodeB), restLength(restLength), springConstant(springConstant),
									dampingConstant(dampingConstant)
			{};
			void calculateForce(DirVec& forceA, DirVec& forceB)
			{
				PosVec posA = nodeA->position;
				PosVec posB = nodeB->position;
				DirVec vecAB = posB - posA;
				Scalar intensity = -1 * springConstant * (restLength - vecAB.length());
				vecAB.normalize();
				DirVec springForceA = intensity * vecAB;
				DirVec velA = nodeA->velocity;
				DirVec velB = nodeB->velocity;
				DirVec velDiff = velA - velB;
				DirVec dampingForceA;
				if (1)
					dampingForceA = -1 * velDiff * dampingConstant;
				else
					dampingForceA = -1 * (velDiff.dot(vecAB) * vecAB) * dampingConstant;
				forceA = springForceA + dampingForceA;
				forceB = -forceA;
			}

			Particle* nodeA;
			Particle* nodeB;
			Scalar restLength;
			Scalar springConstant;
			Scalar dampingConstant;
		};
	} /* namespace base_types */
} /* namespace quarks */
#endif /* SPRING_H_ */
