/*
* PSCF - Polymer Self-Consistent Field Theory
*
* Copyright 2016, The Regents of the University of Minnesota
* Distributed under the terms of the GNU General Public License.
*/

#include "UnitCell.h"
#include <util/math/Constants.h>

namespace Pscf{
namespace Pspg
{

   using namespace Util;

   /*
   * Constructor.
   */
   UnitCell<2>::UnitCell()
    : lattice_(Null)
   {
      dkkBasis_ = new cufftReal[6 * 2 * 2];
      drrBasis_ = new cufftReal[6 * 2 * 2]; 
      cudaMalloc((void**)&dkkBasis_d, 6 * 2 * 2 * sizeof(cufftReal));
   }

   /*
   * Read the lattice system and set nParameter.
   */
   void UnitCell<2>::setNParameter()
   {
      UTIL_CHECK(lattice_ != UnitCell<2>::Null);
      if (lattice_ == UnitCell<2>::Square) {
         nParameter_ = 1;
      } else
      if (lattice_ == UnitCell<2>::Rhombic) {
         nParameter_ = 1;
      } else
      if (lattice_ == UnitCell<2>::Hexagonal) {
         nParameter_ = 1;
      } else
      if (lattice_ == UnitCell<2>::Rectangular) {
         nParameter_ = 2;
      } else
      if (lattice_ == UnitCell<2>::Oblique) {
         nParameter_ = 3;
      } else {
         UTIL_THROW("Invalid lattice system value");
      }
   }

   /*
   * Set the Bravais and reciprocal lattice vectors.
   */
   void UnitCell<2>::setBasis()
   {
      UTIL_CHECK(lattice_ != UnitCell<2>::Null);
      UTIL_CHECK(nParameter_ > 0);

      double twoPi = 2.0*Constants::Pi;
      int i;
      if (lattice_ == UnitCell<2>::Square) {
         UTIL_CHECK(nParameter_ == 1);
         for (i=0; i < 2; ++i) { 
            rBasis_[i][i] = parameters_[0];
            kBasis_[i][i] = twoPi/parameters_[0];
            drBasis_[0](i,i) = 1.0;
         }
      } else 
      if (lattice_ == UnitCell<2>::Rectangular) {
         UTIL_CHECK(nParameter_ == 2);
         for (i=0; i < 2; ++i) { 
            rBasis_[i][i] = parameters_[i];
            kBasis_[i][i] = twoPi/parameters_[i];
            drBasis_[i](i,i) = 1.0;
         }
      } else {
         UTIL_THROW("Unimplemented 2D lattice type");
      }
   }

   /*
   * Extract a UnitCell<2>::LatticeSystem from an istream as a string.
   */
   std::istream& operator >> (std::istream& in,
                              UnitCell<2>::LatticeSystem& lattice)
   {

      std::string buffer;
      in >> buffer;
      if (buffer == "Square" || buffer == "square") {
         lattice = UnitCell<2>::Square;
      } else
      if (buffer == "Rectangular" || buffer == "rectangular") {
         lattice = UnitCell<2>::Rectangular;
      } else
      if (buffer == "Rhombic" || buffer == "rhombic") {
         lattice = UnitCell<2>::Rhombic;
      } else
      if (buffer == "Hexagonal" || buffer == "hexagonal") {
         lattice = UnitCell<2>::Hexagonal;
      } else
      if (buffer == "Oblique" || buffer == "oblique") {
         lattice = UnitCell<2>::Oblique;
      } else {
         UTIL_THROW("Invalid UnitCell<2>::LatticeSystem value input");
      }
      return in;
   }

   /*
   * Insert a UnitCell<2>::LatticeSystem to an ostream as a string.
   */
   std::ostream& operator << (std::ostream& out,
                              UnitCell<2>::LatticeSystem lattice)
   {
      if (lattice == UnitCell<2>::Square) {
         out << "square";
      } else
      if (lattice == UnitCell<2>::Rectangular) {
         out << "rectangular";
      } else
      if (lattice == UnitCell<2>::Rhombic) {
         out << "rhombic";
      } else
      if (lattice == UnitCell<2>::Hexagonal) {
         out << "hexagonal";
      } else
      if (lattice == UnitCell<2>::Oblique) {
         out << "oblique";
      } else {
         UTIL_THROW("This should never happen");
      }
      return out;
   }

}
}
