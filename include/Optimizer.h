#ifndef _OPTIMIZER_H_
#define _OPTIMIZER_H_

#include "Types.h"
#include "TetraMesh.h"
#include "Models.h"
#include "ObjState.h"
#include "Dynamic.h"


namespace BalloonFEM
{
	class OptState : public ObjState
	{
		public:
			OptState(){};
			OptState(TetraMesh* tetra): ObjState(tetra){};

			void update(SpVec dpos);

			size_t freedomDegree();
	};

    class Optimizer : public Engine
    {
        public:
            Optimizer():Engine(){};
            Optimizer(
				TetraMesh* tetra, 
				TetraMesh* target,
				ElasticModel* model = new Elastic_neohookean(0.4, 0.4), 
				AirModel* a_model = new AirModel_Isobaric(0, 0), 
				FilmModel* film_model = new Film_neohookean_3d(0.4, 0.4),
                BendingModel* bend_model = new Bending_MeanCurvature(0.01));

            void solveOptimal();

			void testFunc();
			TetraMesh* Target(){ return m_target;; }

			void setCoeff(Vec3 coeff){ m_alpha = coeff.x; m_beta = coeff.y; m_gamma = coeff.z; };
        protected:

			void computeThicknessLap();
			void computeFilmForces(ObjState &state, Vvec3 &f_sum, SpMat& Tri);
            void computeForceAndGradient(ObjState &state, ObjState &target, SpVec &f, SpMat &A);

			TetraMesh* m_target;
			ObjState* target_state;

			SpMat m_L;
            double m_alpha = 10.0;
            double m_beta = 1.0;
            double m_gamma = 100.0;
			double m_penalty = 10;
    };


}


#endif //! _OPTIMIZER_H_
