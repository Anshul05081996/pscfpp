#ifndef PSPG_POLYMER_H
#define PSPG_POLYMER_H

/*
* PSCF - Polymer Self-Consistent Field Theory
*
* Copyright 2016 - 2019, The Regents of the University of Minnesota
* Distributed under the terms of the GNU General Public License.
*/

#include "Block.h"
#include <pspg/field/RDField.h>
#include <pscf/solvers/PolymerTmpl.h>
#include <util/containers/FArray.h> 

namespace Pscf { 
namespace Pspg { 

   /**
   * Descriptor and solver for a branched polymer species.
   *
   * The block concentrations stored in the constituent Block<D>
   * objects contain the block concentrations (i.e., volume 
   * fractions) computed in the most recent call of the compute 
   * function.
   *
   * The phi() and mu() accessor functions, which are inherited 
   * from PolymerTmp< Block<D> >, return the value of phi (spatial 
   * average volume fraction of a species) or mu (chemical
   * potential) computed in the last call of the compute function.
   * If the ensemble for this species is closed, phi is read from 
   * the parameter file and mu is computed. If the ensemble is
   * open, mu is read from the parameter file and phi is computed.
   *
   * \ingroup Pspg_Solvers_Module
   */
   template <int D>
   class Polymer : public PolymerTmpl< Block<D> >
   {

   public:

      typedef PolymerTmpl< Block<D> > Base;

      typedef typename Block<D>::WField  WField;

      Polymer();

      ~Polymer();

      void setPhi(double phi);

      void setMu(double mu);

      /**
      * Compute solution to MDE and concentrations.
      */ 
      void setupUnitCell(UnitCell<D> const & unitCell, const WaveList<D>& wavelist);

      /**
      * Compute solution to MDE and concentrations.
      */ 
      void compute(DArray<WField> const & wFields);

      /**
      * Compute stress from a polymer chain, needs a pointer to basis
      */
      void computeStress(WaveList<D>& wavelist);

      /**
      * Get derivative of free energy w/ respect to a unit cell parameter.
      *
      * Get the contribution from this polymer species to the derivative of
      * free energy per monomer with respect to unit cell parameter n.
      *
      * \param n unit cell parameter index
      */
      double stress(int n);

      // public inherited functions with non-dependent names
      using Base::nBlock;
      using Base::block;
      using Base::ensemble;
      using Base::solve;
      using Base::length;

   protected:

      // protected inherited function with non-dependent names
      using ParamComposite::setClassName;

   private: 

      // Stress contribution from this polymer species.
      FArray<double, 6> stress_;
     
      // Number of unit cell parameters. 
      int nParams_;

      using Base::phi_;
      using Base::mu_;

   };

   template <int D>
   double Polymer<D>::stress(int n)
   {  return stress_[n]; }

   #ifndef PSPG_POLYMER_TPP
   // Suppress implicit instantiation
   extern template class Polymer<1>;
   extern template class Polymer<2>;
   extern template class Polymer<3>;
   #endif

}
}
//#include "Polymer.tpp"
#endif
