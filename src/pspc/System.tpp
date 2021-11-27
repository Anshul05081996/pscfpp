#ifndef PSPC_SYSTEM_TPP
#define PSPC_SYSTEM_TPP

/*
* PSCF - Polymer Self-Consistent Field Theory
*
* Copyright 2016 - 2019, The Regents of the University of Minnesota
* Distributed under the terms of the GNU General Public License.
*/

#include "System.h"

#include <pspc/sweep/Sweep.h>
#include <pspc/sweep/SweepFactory.h>

#include <pspc/iterator/Iterator.h>
#include <pspc/iterator/IteratorFactory.h>

#include <pspc/solvers/Mixture.h>
#include <pspc/solvers/Polymer.h>
#include <pspc/solvers/Solvent.h>

#include <pscf/mesh/MeshIterator.h>
#include <pscf/crystal/shiftToMinimum.h>
#include <pscf/inter/Interaction.h>
#include <pscf/inter/ChiInteraction.h>
#include <pscf/homogeneous/Clump.h>

#include <util/format/Str.h>
#include <util/format/Int.h>
#include <util/format/Dbl.h>

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <unistd.h>

namespace Pscf {
namespace Pspc
{

   using namespace Util;

   /*
   * Constructor.
   */
   template <int D>
   System<D>::System()
    : mixture_(),
      domain_(),
      fileMaster_(),
      homogeneous_(),
      interactionPtr_(0),
      iteratorPtr_(0),
      iteratorFactoryPtr_(0),
      sweepPtr_(0),
      sweepFactoryPtr_(0),
      wFields_(),
      cFields_(),
      f_(),
      c_(),
      fHelmholtz_(0.0),
      pressure_(0.0),
      hasMixture_(false),
      isAllocated_(false),
      hasWFields_(false),
      hasCFields_(false),
      hasSweep_(false)
   {  
      setClassName("System"); 
      domain_.setFileMaster(fileMaster_);
      interactionPtr_ = new ChiInteraction(); 
      iteratorFactoryPtr_ = new IteratorFactory<D>(*this); 
      sweepFactoryPtr_ = new SweepFactory<D>(*this);
   }

   /*
   * Destructor.
   */
   template <int D>
   System<D>::~System()
   {
      if (interactionPtr_) {
         delete interactionPtr_;
      }
      if (iteratorPtr_) {
         delete iteratorPtr_;
      }
      if (iteratorFactoryPtr_) {
         delete iteratorFactoryPtr_;
      }
      if (sweepPtr_) {
         delete sweepPtr_;
      }
      if (sweepFactoryPtr_) {
         delete sweepFactoryPtr_;
      }
   }

   /*
   * Process command line options.
   */
   template <int D>
   void System<D>::setOptions(int argc, char **argv)
   {
      bool eflag = false;  // echo
      bool pFlag = false;  // param file 
      bool cFlag = false;  // command file 
      bool iFlag = false;  // input prefix
      bool oFlag = false;  // output prefix
      char* pArg = 0;
      char* cArg = 0;
      char* iArg = 0;
      char* oArg = 0;
   
      // Read program arguments
      int c;
      opterr = 0;
      while ((c = getopt(argc, argv, "er:p:c:i:o:f")) != -1) {
         switch (c) {
         case 'e':
            eflag = true;
            break;
         case 'p': // parameter file
            pFlag = true;
            pArg  = optarg;
            break;
         case 'c': // command file
            cFlag = true;
            cArg  = optarg;
            break;
         case 'i': // input prefix
            iFlag = true;
            iArg  = optarg;
            break;
         case 'o': // output prefix
            iFlag = true;
            oArg  = optarg;
            break;
         case '?':
           Log::file() << "Unknown option -" << optopt << std::endl;
           UTIL_THROW("Invalid command line option");
         }
      }
   
      // Set flag to echo parameters as they are read.
      if (eflag) {
         Util::ParamComponent::setEcho(true);
      }

      // If option -p, set parameter file name
      if (pFlag) {
         fileMaster().setParamFileName(std::string(pArg));
      }

      // If option -c, set command file name
      if (cFlag) {
         fileMaster().setCommandFileName(std::string(cArg));
      }

      // If option -i, set path prefix for input files
      if (iFlag) {
         fileMaster().setInputPrefix(std::string(iArg));
      }

      // If option -o, set path prefix for output files
      if (oFlag) {
         fileMaster().setOutputPrefix(std::string(oArg));
      }

   }

   /*
   * Read parameters and initialize.
   */
   template <int D>
   void System<D>::readParameters(std::istream& in)
   {
      readParamComposite(in, mixture());
      hasMixture_ = true;

      int nm = mixture().nMonomer(); 
      int np = mixture().nPolymer(); 
      int ns = mixture().nSolvent(); 

      // Initialize homogeneous object
      homogeneous_.setNMolecule(np+ns);
      homogeneous_.setNMonomer(nm);
      initHomogeneous();

      interaction().setNMonomer(mixture().nMonomer());
      readParamComposite(in, interaction());

      readParamComposite(in, domain_);

      #if 0
      read(in, "unitCell", unitCell_);
      hasUnitCell_ = true;
     
      read(in, "mesh", mesh_);
      fft_.setup(mesh_.dimensions());
      hasMesh_ = true;

      read(in, "groupName", groupName_);

      basis().makeBasis(mesh(), unitCell(), groupName_);
      #endif

      mixture().setMesh(mesh());
      mixture().setupUnitCell(unitCell());

      allocate();
      isAllocated_ = true;

      // Initialize iterator
      std::string className;
      bool isEnd;
      iteratorPtr_ 
         = iteratorFactoryPtr_->readObject(in, *this, className, isEnd);
      if (!iteratorPtr_) {
         std::string msg = "Unrecognized Iterator subclass name ";
         msg += className;
         UTIL_THROW(msg.c_str());
      }
      iterator().setup();

      #if 0
      // Optionally instantiate a Sweep object
      readOptional<bool>(in, "hasSweep", hasSweep_);
      if (hasSweep_) {
         bool isEnd;
         sweepPtr_ = 
            sweepFactoryPtr_->readObject(in, *this, className, isEnd);
         if (!sweepPtr_) {
            UTIL_THROW("Unrecognized Sweep subclass name");
         }
         sweepPtr_->setSystem(*this);
      }
      #endif
   }

   /*
   * Read default parameter file.
   */
   template <int D>
   void System<D>::readParam(std::istream& in)
   {
      readBegin(in, className().c_str());
      readParameters(in);  
      readEnd(in);
   }

   /*
   * Read default parameter file.
   */
   template <int D>
   void System<D>::readParam()
   {  readParam(fileMaster().paramFile()); }

   /*
   * Allocate memory for fields.
   */
   template <int D>
   void System<D>::allocate()
   {
      // Preconditions
      UTIL_CHECK(hasMixture_);

      // Allocate wFields and cFields
      int nMonomer = mixture().nMonomer();
      wFields_.allocate(nMonomer);
      wFieldsRGrid_.allocate(nMonomer);
      wFieldsKGrid_.allocate(nMonomer);

      cFields_.allocate(nMonomer);
      cFieldsRGrid_.allocate(nMonomer);
      cFieldsKGrid_.allocate(nMonomer);
      
      tmpFields_.allocate(nMonomer);
      tmpFieldsRGrid_.allocate(nMonomer);
      tmpFieldsKGrid_.allocate(nMonomer);

      for (int i = 0; i < nMonomer; ++i) {
         wField(i).allocate(basis().nStar());
         wFieldRGrid(i).allocate(mesh().dimensions());
         wFieldKGrid(i).allocate(mesh().dimensions());

         cField(i).allocate(basis().nStar());
         cFieldRGrid(i).allocate(mesh().dimensions());
         cFieldKGrid(i).allocate(mesh().dimensions());

         tmpFields_[i].allocate(basis().nStar());
         tmpFieldsRGrid_[i].allocate(mesh().dimensions());
         tmpFieldsKGrid_[i].allocate(mesh().dimensions());
      }
      isAllocated_ = true;
   }

   /*
   * Read a filename string and echo to log file (used in readCommands).
   */
   template <int D>
   void System<D>::readEcho(std::istream& in, std::string& string) const
   {
      in >> string;
      Log::file() << " " << Str(string, 20) << std::endl;
   }

   
   /*
   * Read and execute commands from a specified command file.
   */
   template <int D>
   void System<D>::readCommands(std::istream &in) 
   {
      UTIL_CHECK(isAllocated_);
      std::string command, filename, inFileName, outFileName;

      bool readNext = true;
      while (readNext) {

         in >> command;
         Log::file() << command <<std::endl;

         if (command == "FINISH") {
            Log::file() << std::endl;
            readNext = false;
         } else
         if (command == "READ_W_BASIS") {
            readEcho(in, filename);
            readWBasis(filename);
         } else
         if (command == "READ_W_RGRID") {
            readEcho(in, filename);
            readWRGrid(filename);
         } else
         if (command == "ITERATE") {
            // Read w (chemical potential) fields if not done previously 
            if (!hasWFields_) {
               readEcho(in, filename);
               readWBasis(filename);
            }
            // Attempt iteration to convergence
            int fail = iterate();
            if (fail) {
               readNext = false;
            }
         } else
         if (command == "SOLVE_MDE") {
            // Read w (chemical potential fields) if not done previously 
            if (!hasWFields_) {
               readEcho(in, filename);
               readWBasis(filename);
            }
            // Solve the modified diffusion equation, without iteration
            compute();
         } else
         if (command == "WRITE_W_BASIS") {
            readEcho(in, filename);
            writeWBasis(filename);
         } else 
         if (command == "WRITE_W_RGRID") {
            readEcho(in, filename);
            writeWRGrid(filename);
         } else 
         if (command == "WRITE_C_BASIS") {
            readEcho(in, filename);
            writeCBasis(filename);
         } else
         if (command == "WRITE_C_RGRID") {
            readEcho(in, filename);
            writeCRGrid(filename);
         } else
         if (command == "BASIS_TO_RGRID") {
            readEcho(in, inFileName);
            readEcho(in, outFileName);
            basisToRGrid(inFileName, outFileName);
         } else 
         if (command == "RGRID_TO_BASIS") {
            readEcho(in, inFileName);
            readEcho(in, outFileName);
            rGridToBasis(inFileName, outFileName);
         } else
         if (command == "KGRID_TO_RGRID") {
            readEcho(in, inFileName);
            readEcho(in, outFileName);
            kGridToRGrid(inFileName, outFileName);
         } else
         if (command == "RGRID_TO_KGRID") {
            readEcho(in, inFileName);
            readEcho(in, outFileName);
            rGridToKGrid(inFileName, outFileName);
         } else
         if (command == "CHECK_RGRID_SYMMETRY") {
            readEcho(in, inFileName);
            checkRGridFieldSymmetry(inFileName);
         } else
         if (command == "RHO_TO_OMEGA") {
            readEcho(in, inFileName);
            readEcho(in, outFileName);
            rhoToOmega(inFileName, outFileName);
         } else
         if (command == "OUTPUT_STARS") {
            readEcho(in, outFileName);
            outputStars(outFileName);
         } else
         if (command == "OUTPUT_WAVES") {
            readEcho(in, outFileName);
            outputWaves(outFileName);
         } else {
            Log::file() << "Error: Unknown command  " 
                        << command << std::endl;
            readNext = false;
         }
      }
   }

   /*
   * Read and execute commands from the default command file.
   */
   template <int D>
   void System<D>::readCommands()
   {  
      if (fileMaster().commandFileName().empty()) {
         UTIL_THROW("Empty command file name");
      }
      readCommands(fileMaster().commandFile()); 
   }

  
   /*
   * Compute Helmoltz free energy and pressure
   */
   template <int D>
   void System<D>::computeFreeEnergy()
   {
      UTIL_CHECK(hasWFields_);
      UTIL_CHECK(hasCFields_);

      // Initialize to zero
      fHelmholtz_ = 0.0;
 
      double phi, mu;
      int np = mixture().nPolymer();
      int ns = mixture().nSolvent();

      // Compute polymer ideal gas contributions to fHelhmoltz_
      if (np > 0) {
         Polymer<D>* polymerPtr;
         double length;
         for (int i = 0; i < np; ++i) {
            polymerPtr = &mixture().polymer(i);
            phi = polymerPtr->phi();
            mu = polymerPtr->mu();
            length = polymerPtr->length();
            // Recall: mu = ln(phi/q)
            fHelmholtz_ += phi*( mu - 1.0 )/length;
         }
      }

      // Compute solvent ideal gas contributions to fHelhmoltz_
      if (ns > 0) {
         Solvent<D>* solventPtr;
         double size;
         for (int i = 0; i < ns; ++i) {
            solventPtr = &mixture().solvent(i);
            phi = solventPtr->phi();
            mu = solventPtr->mu();
            size = solventPtr->size();
            fHelmholtz_ += phi*( mu - 1.0 )/size;
         }
      }

      int nm  = mixture().nMonomer();
      int nStar = basis().nStar();

      // Compute Legendre transform subtraction
      // Use expansion in symmetry-adapted orthonormal basis
      double temp = 0.0;
      for (int i = 0; i < nm; ++i) {
         for (int k = 0; k < nStar; ++k) {
            temp += wFields_[i][k] * cFields_[i][k];
         }
      }
      fHelmholtz_ -= temp;

      // Compute excess interaction free energy [ phi^{T}*chi*phi ]
      double chi;
      for (int i = 0; i < nm; ++i) {
         for (int j = i + 1; j < nm; ++j) {
            chi = interaction().chi(i,j);
            for (int k = 0; k < nStar; ++k) {
               fHelmholtz_+= chi * cFields_[i][k] * cFields_[j][k];
            }
         }
      }

      // Initialize pressure
      pressure_ = -fHelmholtz_;

      // Polymer corrections to pressure
      if (np > 0) {
         Polymer<D>* polymerPtr;
         double length;
         for (int i = 0; i < np; ++i) {
            polymerPtr = &mixture().polymer(i);
            phi = polymerPtr->phi();
            mu = polymerPtr->mu();
            length = polymerPtr->length();
            pressure_ += mu * phi /length;
         }
      }

      // Solvent corrections to pressure
      if (ns > 0) {
         Solvent<D>* solventPtr;
         double size;
         for (int i = 0; i < ns; ++i) {
            solventPtr = &mixture().solvent(i);
            phi = solventPtr->phi();
            mu = solventPtr->mu();
            size = solventPtr->size();
            pressure_ += mu * phi /size;
         }
      }

   }

   template <int D>
   void System<D>::outputThermo(std::ostream& out)
   {
      out << std::endl;
      out << "fHelmholtz = " << Dbl(fHelmholtz(), 18, 11) << std::endl;
      out << "pressure   = " << Dbl(pressure(), 18, 11) << std::endl;
      out << std::endl;

      int np = mixture().nPolymer();
      int ns = mixture().nSolvent();

      if (np > 0) {
         out << "Polymers:" << std::endl;
         out << "    i"
             << "        phi[i]      "
             << "        mu[i]       " 
             << std::endl;
         for (int i = 0; i < np; ++i) {
            out << Int(i, 5) 
                << "  " << Dbl(mixture().polymer(i).phi(),18, 11)
                << "  " << Dbl(mixture().polymer(i).mu(), 18, 11)  
                << std::endl;
         }
         out << std::endl;
      }

      if (ns > 0) {
         out << "Solvents:" << std::endl;
         out << "    i"
             << "        phi[i]      "
             << "        mu[i]       " 
             << std::endl;
         for (int i = 0; i < ns; ++i) {
            out << Int(i, 5) 
                << "  " << Dbl(mixture().solvent(i).phi(),18, 11)
                << "  " << Dbl(mixture().solvent(i).mu(), 18, 11)  
                << std::endl;
         }
         out << std::endl;
      }

   }

   template <int D>
   void System<D>::initHomogeneous()
   {

      // Set number of molecular species and monomers
      int nm = mixture().nMonomer(); 
      int np = mixture().nPolymer(); 
      int ns = mixture().nSolvent(); 
      UTIL_CHECK(homogeneous_.nMolecule() == np + ns);
      UTIL_CHECK(homogeneous_.nMonomer() == nm);

      // Allocate c_ work array, if necessary
      if (c_.isAllocated()) {
         UTIL_CHECK(c_.capacity() == nm);
      } else {
         c_.allocate(nm);
      }

      int i;   // molecule index
      int j;   // monomer index
      int k;   // block or clump index
      int nb;  // number of blocks
      int nc;  // number of clumps
 
      // Loop over polymer molecule species
      if (np > 0) {
         for (i = 0; i < np; ++i) {
   
            // Initial array of clump sizes 
            for (j = 0; j < nm; ++j) {
               c_[j] = 0.0;
            }
   
            // Compute clump sizes for all monomer types.
            nb = mixture().polymer(i).nBlock(); 
            for (k = 0; k < nb; ++k) {
               Block<D>& block = mixture().polymer(i).block(k);
               j = block.monomerId();
               c_[j] += block.length();
            }
    
            // Count the number of clumps of nonzero size
            nc = 0;
            for (j = 0; j < nm; ++j) {
               if (c_[j] > 1.0E-8) {
                  ++nc;
               }
            }
            homogeneous_.molecule(i).setNClump(nc);
    
            // Set clump properties for this Homogeneous::Molecule
            k = 0; // Clump index
            for (j = 0; j < nm; ++j) {
               if (c_[j] > 1.0E-8) {
                  homogeneous_.molecule(i).clump(k).setMonomerId(j);
                  homogeneous_.molecule(i).clump(k).setSize(c_[j]);
                  ++k;
               }
            }
            homogeneous_.molecule(i).computeSize();
   
         }
      }

      // Add solvent contributions
      if (np > 0) {
         double size;
         int monomerId;
         for (int is = 0; is < ns; ++is) {
            i = is + np;
            monomerId = mixture().solvent(is).monomerId();
            size = mixture().solvent(is).size();
            homogeneous_.molecule(i).setNClump(1);
            homogeneous_.molecule(i).clump(0).setMonomerId(monomerId);
            homogeneous_.molecule(i).clump(0).setSize(size);
            homogeneous_.molecule(i).computeSize();
         }
      }

   }

   // Command functions

   /*
   * Read w-field in symmetry adapted basis format.
   */
   template <int D>
   void System<D>::readWBasis(const std::string & filename)
   {
      fieldIo().readFieldsBasis(filename, wFields(), unitCell());
      fieldIo().convertBasisToRGrid(wFields(), wFieldsRGrid());
      hasWFields_ = true;
      hasCFields_ = false;
   }

   /*
   * Read w-fields in real-space grid (r-grid) format.
   */
   template <int D>
   void System<D>::readWRGrid(const std::string & filename)
   {
      fieldIo().readFieldsRGrid(filename, wFieldsRGrid(), unitCell());
      fieldIo().convertRGridToBasis(wFieldsRGrid(), wFields());
      hasWFields_ = true;
      hasCFields_ = false;
   }

   /*
   * Iteratively solve a SCFT problem for specified parameters.
   */
   template <int D>
   int System<D>::iterate()
   {
      UTIL_CHECK(hasWFields_);
      hasCFields_ = false;

      Log::file() << std::endl;
      Log::file() << std::endl;

      // Call iterator
      int error = iterator().solve();
      hasCFields_ = true;

      if (error) {
         Log::file() << "Iterator failed to converge\n";
      } else {
         computeFreeEnergy();
         outputThermo(Log::file());
      }
      return error;
   }

   /*
   * Solve MDE for current w-fields, without iteration.
   */
   template <int D>
   void System<D>::compute()
   {
      UTIL_CHECK(hasWFields_);

      // Solve the modified diffusion equation (without iteration)
      mixture().compute(wFieldsRGrid(), cFieldsRGrid());

      // Convert c fields from r-grid to basis
      fieldIo().convertRGridToBasis(cFieldsRGrid(), cFields());
      hasCFields_ = true;
   }

   /*
   * Write w-fields in symmetry-adapted basis format. 
   */
   template <int D>
   void System<D>::writeWBasis(const std::string & filename)
   {
      UTIL_CHECK(hasWFields_);
      fieldIo().writeFieldsBasis(filename, wFields(), unitCell());
   }

   /*
   * Write w-fields to real space grid.
   */
   template <int D>
   void System<D>::writeWRGrid(const std::string & filename)
   {
      UTIL_CHECK(hasWFields_);
      fieldIo().writeFieldsRGrid(filename, wFieldsRGrid(), unitCell());
   }

   /*
   * Write all concentration fields in symmetry-adapted basis format.
   */
   template <int D>
   void System<D>::writeCBasis(const std::string & filename)
   {
      UTIL_CHECK(hasCFields_);
      fieldIo().writeFieldsBasis(filename, cFields(), unitCell());
   }

   /*
   * Write all concentration fields in real space (r-grid) format.
   */
   template <int D>
   void System<D>::writeCRGrid(const std::string & filename)
   {
      UTIL_CHECK(hasCFields_);
      fieldIo().writeFieldsRGrid(filename, cFieldsRGrid(), unitCell());
   }

   // Field conversion command functions

   /*
   * Convert fields from symmetry-adpated basis to real-space grid format.
   */
   template <int D>
   void System<D>::basisToRGrid(const std::string & inFileName,
                                const std::string & outFileName)
   {
      UnitCell<D> tmpUnitCell;
      fieldIo().readFieldsBasis(inFileName, tmpFields_, tmpUnitCell);
      fieldIo().convertBasisToRGrid(tmpFields_, tmpFieldsRGrid_);
      fieldIo().writeFieldsRGrid(outFileName, tmpFieldsRGrid_, 
                                 tmpUnitCell);
   }

   /*
   * Convert fields from real-space grid to symmetry-adapted basis format.
   */
   template <int D>
   void System<D>::rGridToBasis(const std::string & inFileName,
                                const std::string & outFileName)
   {
      UnitCell<D> tmpUnitCell;
      fieldIo().readFieldsRGrid(inFileName, tmpFieldsRGrid_, tmpUnitCell);
      fieldIo().convertRGridToBasis(tmpFieldsRGrid_, tmpFields_);
      fieldIo().writeFieldsBasis(outFileName, tmpFields_, tmpUnitCell);
   }

   /*
   * Convert fields from Fourier (k-grid) to real-space (r-grid) format.
   */
   template <int D>
   void System<D>::kGridToRGrid(const std::string & inFileName,
                                const std::string& outFileName)
   {
      UnitCell<D> tmpUnitCell;
      fieldIo().readFieldsKGrid(inFileName, tmpFieldsKGrid_, tmpUnitCell);
      for (int i = 0; i < mixture().nMonomer(); ++i) {
         fft().inverseTransform(tmpFieldsKGrid_[i], tmpFieldsRGrid_[i]);
      }
      fieldIo().writeFieldsRGrid(outFileName, tmpFieldsRGrid_, 
                                 tmpUnitCell);
   }

   /*
   * Convert fields from real-space (r-grid) to Fourier (k-grid) format.
   */
   template <int D>
   void System<D>::rGridToKGrid(const std::string & inFileName,
                                const std::string & outFileName)
   {
      UnitCell<D> tmpUnitCell;
      fieldIo().readFieldsRGrid(inFileName, tmpFieldsRGrid_, 
                                tmpUnitCell);
      for (int i = 0; i < mixture().nMonomer(); ++i) {
         fft().forwardTransform(tmpFieldsRGrid_[i], tmpFieldsKGrid_[i]);
      }
      fieldIo().writeFieldsKGrid(outFileName, tmpFieldsKGrid_, 
                                 tmpUnitCell);
   }

   /*
   * Convert fields from real-space grid to symmetry-adapted basis format.
   */
   template <int D>
   bool System<D>::checkRGridFieldSymmetry(const std::string & inFileName)
   {
      UnitCell<D> tmpUnitCell;
      fieldIo().readFieldsRGrid(inFileName, tmpFieldsRGrid_, tmpUnitCell);
      for (int i = 0; i < mixture().nMonomer(); ++i) {
         bool symmetric = fieldIo().hasSymmetry(tmpFieldsRGrid_[i]);
         if (!symmetric) {
            return false;
         }
      }
      return true;
   }

   /*
   * Construct guess for omega (w-field) from rho (c-field).
   *
   * Modifies wFields and wFieldsRGrid and outputs wFields.
   */
   template <int D>
   void System<D>::rhoToOmega(const std::string & inFileName, 
                              const std::string & outFileName)
   {
      hasCFields_ = false;
      hasWFields_ = false;

      fieldIo().readFieldsBasis(inFileName, tmpFields_, unitCell());

      #if 0
      // Open input file
      std::ifstream inFile;
      fileMaster().openInputFile(inFileName, inFile);

      // Read concentration field
      std::string label;
      int nStar,nM;
      inFile >> label;
      inFile >> nStar;
      inFile >> label;
      inFile >> nM;
      int idum;
      for (int i = 0; i < nStar; ++i) {
         inFile >> idum;
         for (int j = 0; j < nM; ++j) {
            inFile >> tmpFields_[j][i];
         }
      }
      inFile.close();
      #endif

      // Compute w fields from c fields
      for (int i = 0; i < basis().nStar(); ++i) {
         for (int j = 0; j < mixture().nMonomer(); ++j) {
            wField(j)[i] = 0;
            for (int k = 0; k < mixture().nMonomer(); ++k) {
               wField(j)[i] += interaction().chi(j,k) * tmpFields_[k][i];
            }
         }
      }

      // Convert to r-grid format
      fieldIo().convertBasisToRGrid(wFields(), wFieldsRGrid());
      hasWFields_ = true;
      hasCFields_ = false;

      // Write w field in basis format
      fieldIo().writeFieldsBasis(outFileName, wFields(), unitCell());
   }

   /*
   * Write description of symmetry-adapted stars and basis to file.
   */
   template <int D>
   void System<D>::outputStars(const std::string & outFileName)
   {
      std::ofstream outFile;
      fileMaster().openOutputFile(outFileName, outFile);
      fieldIo().writeFieldHeader(outFile, mixture().nMonomer(),
                                 unitCell());
      basis().outputStars(outFile);
   }

   /*
   * Write a list of waves and associated stars to file.
   */
   template <int D>
   void System<D>::outputWaves(const std::string & outFileName)
   {
      std::ofstream outFile;
      fileMaster().openOutputFile(outFileName, outFile);
      fieldIo().writeFieldHeader(outFile, mixture().nMonomer(), 
                                 unitCell());
      basis().outputWaves(outFile);
   }

} // namespace Pspc
} // namespace Pscf
#endif
