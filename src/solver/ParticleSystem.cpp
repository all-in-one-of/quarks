#include <boost/numeric/odeint.hpp>
#include <CGAL/Cartesian_d.h>
#include <CGAL/point_generators_d.h>
#include <openvdb/math/Operators.h>
#include <openvdb/math/FiniteDifference.h>
#include "OdeSolver.h"
#include <thread>
#include <math.h>
#include "ParticleSystem.h"
typedef CGAL::Cartesian_d<double> Kd;
typedef Kd::Point_d Point;
namespace quarks
{
	namespace solver
	{
		ParticleSystem::ParticleSystem()
		{
			particles.reserve(MAX_NUM_PARTICLES);
			collision = NULL;
			IsCollisionRegistered = false;
			clothSolverFlag = false;
			initializeSystem();
		}
		void ParticleSystem::initializeSystem()
		{
			for (int i = 0; i < particles.size(); i++)
			{
				delete particles[i];
			}
			particles.clear();
			for (int i = 0; i < springs.size(); i++)
			{
				delete springs[i];
			}
			springs.clear();
			steps = 0;
		}
		bool ParticleSystem::isClothSolverFlag() const
		{
			return clothSolverFlag;
		}

		void ParticleSystem::setClothSolverFlag(bool clothSolverFlag)
		{
			this->clothSolverFlag = clothSolverFlag;
		}

		void ParticleSystem::stepForward(Scalar timeStep)
		{
			sourceManager.birthParticles(particles, steps);
			softBodyManager.birthParticles(particles, springs, steps);
			forceManager.accumulateForces(particles, springs);
			theadedOperation(timeStep);
			steps++;
		}

		void ParticleSystem::theadedOperation(Scalar timeStep)
		{
			int numOfThreads = std::thread::hardware_concurrency() - 1;

			std::vector<std::thread> threadsA;
			for (unsigned i = 0; i < numOfThreads; i++)
			{
				threadsA.push_back(
						std::thread(&ParticleSystem::solveStep, this, i, numOfThreads,
									timeStep));
			}
			for (auto& thread : threadsA)
			{
				thread.join();
			}

			std::vector<std::thread> threadsB;
			for (unsigned i = 0; i < numOfThreads; i++)
			{
				threadsB.push_back(
						std::thread(&ParticleSystem::killOldParticles, this, i, numOfThreads));
			}
			for (auto& thread : threadsB)
			{
				thread.join();
			}
		}

		void ParticleSystem::solveStep(int threadIndex, int numThreads, Scalar timeStep)
		{
			for (int i = threadIndex; i < particles.size(); i += numThreads)
			{
				Particle* partPtr = particles[i];
				if (partPtr == NULL)
				{
					continue;
				}
				PosVec oldPos = partPtr->position;
				DirVec oldVel = partPtr->velocity;
				DirVec oldForce = partPtr->force / partPtr->mass;
				PosVec newPos;
				DirVec newVel;
				OdeSolver odeSolver;

				if (partPtr->isFixed)
				{
					PosVec constraintPos = softBodyManager.getConstraintPos(partPtr->softBodySourceNum,
																			partPtr->softBodyPointNum);
					partPtr->position = constraintPos;
					continue;
				}

				if (clothSolverFlag)
					odeSolver.applyRK4(timeStep, oldPos, oldVel, oldForce, newPos, newVel);
				else
					odeSolver.applyEuler(timeStep, oldPos, oldVel, oldForce, newPos, newVel);

				if (IsCollisionRegistered)
				{
					collision->applyCollision(oldPos, oldVel, newPos, newVel);
				}
				partPtr->position = (newPos);
				partPtr->velocity = (newVel);
				partPtr->life = (partPtr->life + 1);
			}
		}

		void ParticleSystem::killOldParticles(int threadIndex, int numThreads)
		{
			for (int i = threadIndex; i < particles.size(); i += numThreads)
			{
				Particle* partPtr = particles[i];
				if (partPtr == NULL)
				{
					continue;
				}
				if (partPtr->life > partPtr->lifeExpectancy)
				{
					delete partPtr;
					particles[i] = NULL;
				}
			}
		}
	}
} /* namespace quarks */
