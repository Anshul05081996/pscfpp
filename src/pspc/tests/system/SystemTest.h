#ifndef PSPC_SYSTEM_TEST_H
#define PSPC_SYSTEM_TEST_H

#include <test/UnitTest.h>
#include <test/UnitTestRunner.h>

#include <pspc/System.h>
#include <pscf/mesh/MeshIterator.h>

//#include <pspc/iterator/AmIterator.h>
//#include <util/format/Dbl.h>

#include <fstream>

using namespace Util;
using namespace Pscf;
using namespace Pscf::Pspc;

class SystemTest : public UnitTest
{

public:

   std::ofstream logFile_;

   void setUp()
   {}

   void tearDown()
   {
      if (logFile_.is_open()) {
         logFile_.close();
      }
   }

   void openLogFile(char const * filename)
   {  
      openOutputFile(filename, logFile_); 
      Log::setFile(logFile_);
   }

   void testConstructor1D()
   {
      printMethod(TEST_FUNC);
      System<1> system;
   }

   void testReadParameters1D()
   {
      printMethod(TEST_FUNC);
      System<1> system;
      system.fileMaster().setInputPrefix(filePrefix());
      system.fileMaster().setOutputPrefix(filePrefix());

      std::ifstream in;
      openInputFile("in/domainOn/System1D", in);
      system.readParam(in);
      in.close();
   }
 
   void testConversion1D_lam() 
   {   
      printMethod(TEST_FUNC);
      System<1> system;
      system.fileMaster().setInputPrefix(filePrefix());
      system.fileMaster().setOutputPrefix(filePrefix());

      openLogFile("out/testConversion1D_lam.log"); 

      std::ifstream in; 
      openInputFile("in/domainOn/System1D", in);
      system.readParam(in);
      in.close();

      // std::ifstream command;
      // openInputFile("in/conv/Conversion_1d_step1", command);
      // system.readCommands(command);
      // command.close();

      // Read w-fields (reference solution, solved by Fortran PSCF)
      system.readWBasis("contents/omega/domainOn/omega_lam");

      // Copy w field components to wFields_check after reading
      int nMonomer = system.mixture().nMonomer();
      int nStar = system.basis().nStar();
      DArray< RField<1> > wFields_check;
      wFields_check.allocate(nMonomer);
      for (int i = 0; i < nMonomer; ++i){
         wFields_check[i].allocate(nStar);
         for (int j = 0; j < nStar; ++j){    
            wFields_check[i][j] = system.wFields()[i][j];
            system.wFields()[i][j] = 0.0;
         }   
      }   

      // std::ifstream command_2;
      // openInputFile("in/conv/Conversion_1d_step2", command_2);
      // system.readCommands(command_2);
      //command_2.close();

      // Round trip conversion basis -> rgrid -> basis, read result
      system.basisToRGrid("contents/omega/domainOn/omega_lam",
                          "out/omega/conv/omega_rgrid_lam");
      system.rGridToBasis("out/omega/conv/omega_rgrid_lam",
                          "out/omega/conv/omega_conv_lam");
      system.readWBasis("out/omega/conv/omega_conv_lam");

      // Compare result to original
      double err;
      double max = 0.0;
      std::cout << std::endl;   
      for (int j = 0; j < nStar; ++j) {
         for (int i = 0; i < nMonomer; ++i) {
            err = wFields_check[i][j] - system.wFields()[i][j];
            err = std::abs(err);
            //std::cout << Dbl(wFields_check[i][j],15,8)  << "  ";
            //std::cout << Dbl(system.wFields()[i][j],15,8) << "  ";
            if (err > max) {
               max = err;
            }
         }
         //std::cout << std::endl;   
      }
      std::cout << "Max error = " << max << std::endl;  
      TEST_ASSERT(max < 1.0E-8);
   }

   void testConversion2D_hex() 
   {   
      printMethod(TEST_FUNC);
      System<2> system;
      system.fileMaster().setInputPrefix(filePrefix());
      system.fileMaster().setOutputPrefix(filePrefix());

      openLogFile("out/testConversion2D_hex.log"); 

      // Read parameter file
      std::ifstream in; 
      openInputFile("in/domainOn/System2D", in);
      system.readParam(in);
      in.close();

      // std::ifstream command;
      // openInputFile("in/conv/Conversion_2d_step1", command);
      // system.readCommands(command);
      // command.close();

      // Read w fields
      system.readWBasis("contents/omega/domainOn/omega_hex");

      // Store components in wFields_check for later comparison 
      int nMonomer = system.mixture().nMonomer();
      int nStar = system.basis().nStar();
      DArray<RField<2> > wFields_check;
      wFields_check.allocate(nMonomer);
      for (int i = 0; i < nMonomer; ++i) {
         wFields_check[i].allocate(nStar);
         for (int j = 0; j < nStar; ++j){    
            wFields_check[i][j] = system.wFields() [i] [j];
            system.wFields()[i][j] = 0.0;
         }   
      }   

      // Round trip basis -> rgrid -> basis, read resulting wField
      system.basisToRGrid("contents/omega/domainOn/omega_hex",
                          "out/omega/conv/omega_rgrid_hex");
      system.rGridToBasis("out/omega/conv/omega_rgrid_hex",
                          "out/omega/conv/omega_conv_hex");
      system.readWBasis("out/omega/conv/omega_conv_hex");

      // Check symmetry of rgrid representation
      bool hasSymmetry
       = system.checkRGridFieldSymmetry("out/omega/conv/omega_rgrid_hex");
      TEST_ASSERT(hasSymmetry);

      // Compare result to original
      double err;
      double max = 0.0;
      std::cout << std::endl;   
      for (int j = 0; j < nStar; ++j) {
         for (int i = 0; i < nMonomer; ++i) {
            err = wFields_check[i][j] - system.wFields()[i][j];
            err = std::abs(err);
            //std::cout << wFields_check[i][j] 
            //          << "  " << system.wFields()[i][j];
            if (err > max) {
               max = err;
            }
         }
         //std::cout << std::endl;   
      }   
      std::cout << "Max error = " << max << std::endl;  
      TEST_ASSERT(max < 1.0E-9);

   }  

   void testConversion3D_bcc() 
   {   
      printMethod(TEST_FUNC);
      System<3> system;
      system.fileMaster().setInputPrefix(filePrefix());
      system.fileMaster().setOutputPrefix(filePrefix());

      openLogFile("out/testConversion3D_bcc.log"); 

      // Read parameter file
      std::ifstream in; 
      openInputFile("in/domainOn/System3D", in);
      system.readParam(in);
      in.close();

      // openInputFile("in/conv/Conversion_3d_step1", in);
      // system.readCommands(in);
      // in.close();

      // Read w fields in system.wFields
      system.readWBasis("contents/omega/domainOn/omega_bcc");
  
      // Store components of field as input 
      int nMonomer = system.mixture().nMonomer();
      int nStar = system.basis().nStar();
      DArray<RField<3> > wFields_check;
      wFields_check.allocate(nMonomer);
      for (int i = 0; i < nMonomer; ++i){
         wFields_check[i].allocate(nStar);
         for (int j = 0; j < nStar; ++j){         
            wFields_check[i][j] = system.wFields()[i][j];
            system.wFields()[i][j] = 0.0;
         }
      }

      // std::ifstream command_2;
      // openInputFile("in/conv/Conversion_3d_step2", command_2);
      // system.readCommands(command_2);
      // command_2.close();

      // Complete round trip basis -> rgrid -> basis
      system.basisToRGrid("contents/omega/domainOn/omega_bcc",
                          "out/omega/conv/omega_rgrid_bcc");
      system.rGridToBasis("out/omega/conv/omega_rgrid_bcc",
                          "out/omega/conv/omega_conv_bcc");
      system.readWBasis("out/omega/conv/omega_conv_bcc");

      // Check symmetry of rgrid representation
      bool hasSymmetry
       = system.checkRGridFieldSymmetry("out/omega/conv/omega_rgrid_bcc");
      TEST_ASSERT(hasSymmetry);

      // Compare result to original
      double err;
      double max = 0.0;
      std::cout << std::endl;   
      for (int j = 0; j < nStar; ++j) {
         //std::cout << j << "  ";
         for (int i = 0; i < nMonomer; ++i) {
            err = wFields_check[i][j] - system.wFields()[i][j];
            err = std::abs(err);
            //std::cout << Dbl(wFields_check[i][j],15,8)  << "  ";
            //std::cout << Dbl(system.wFields()[i][j],15,8) << "  ";
            if (err > max) {
               max = err;
            }
         }
         //std::cout << std::endl;   
      }
      std::cout << "Max error = " << max << std::endl;  
      TEST_ASSERT(max < 1.0E-8);

   }   

   void testIterate1D_lam_rigid()
   {
      printMethod(TEST_FUNC);
      openLogFile("out/testIterate1D_lam_rigid.log"); 

      System<1> system;
      system.fileMaster().setInputPrefix(filePrefix());
      system.fileMaster().setOutputPrefix(filePrefix());

      std::ifstream in;
      openInputFile("in/domainOff/System1D", in); 
      system.readParam(in);
      in.close();

      // std::ifstream command;
      // openInputFile("in/domainOff/ReadOmega_lam", command);
      // system.readCommands(command);
      // command.close();

      // Read w fields
      system.readWBasis("contents/omega/domainOff/omega_lam");

      int nMonomer = system.mixture().nMonomer();
      DArray<RField<1> > wFields_check;
      DArray<RField<1> > wFields;
      wFields_check.allocate(nMonomer);
      wFields.allocate(nMonomer);
      int ns = system.basis().nStar();
      for (int i = 0; i < nMonomer; ++i) {
          wFields_check[i].allocate(ns);
      }    

      for (int i = 0; i < nMonomer; ++i) {
         for (int j = 0; j < ns; ++j) {
            wFields_check[i][j] = system.wFields() [i] [j]; 
         }    
      }    

      // std::ifstream command_2;
      // openInputFile("in/domainOff/Iterate1d", command_2);
      // system.readCommands(command_2);
      // command_2.close();

      // Read w-fields, iterate and output solution
      system.readWBasis("contents/omega/domainOff/omega_lam");
      system.iterate();
      system.writeWBasis("out/omega/domainOff/omega_lam");

      bool diff = true;
      for (int j = 0; j < ns; ++j) {
         for (int i = 0; i < nMonomer; ++i) {
            if ((std::abs(wFields_check[i][j] - system.wFields()[i][j]) >= 5.07058e-08)){
                // The above is the minimum error in the omega field.
                // Occurs for the first star                 
                diff = false;
                std::cout <<"\n This is error for break:"<<
                   (std::abs(wFields_check[i][j] - system.wFields()[i][j])) <<std::endl;
                std::cout << "star index = " << j << std::endl;
                break;
            } else {
                diff = true;
            }
         }    
         if (diff == false) {
            break;
         }    
      }    
      bool stress = false;
      if (std::abs(system.mixture().stress(0) - 0.006583929) < 1.0E-8) {
         //0.006583929 is the stress calculated 
         //for this omega field for no stress relaxation using Fortran
         stress = true;
      }

      TEST_ASSERT(stress);
      TEST_ASSERT(diff);

   }

   void testIterate1D_lam_flex()
   {
      printMethod(TEST_FUNC);
      openLogFile("out/testIterate1D_lam_.log"); 
 
      System<1> system;
      system.fileMaster().setInputPrefix(filePrefix());
      system.fileMaster().setOutputPrefix(filePrefix());

      std::ifstream in; 
      openInputFile("in/domainOn/System1D", in);
      system.readParam(in);
      in.close();

      // std::ifstream command;
      // openInputFile("in/domainOn/ReadOmega_lam", command);
      // system.readCommands(command);
      // command.close();
      system.readWBasis("contents/omega/domainOn/omega_lam");
 
      int nMonomer = system.mixture().nMonomer();
      DArray<RField<1> > wFields_check;
      DArray<RField<1> > wFields;
      wFields_check.allocate(nMonomer);
      wFields.allocate(nMonomer);
      int ns = system.basis().nStar();
      for (int i = 0; i < nMonomer; ++i) {
          wFields_check[i].allocate(ns);
      }   

      for (int i = 0; i < nMonomer; ++i) {
         for (int j = 0; j < ns; ++j) {    
            wFields_check[i][j] = system.wFields() [i] [j];
         }   
      }   

      // std::ifstream command_2;
      // openInputFile("in/domainOn/Iterate1d", command_2);
      // system.readCommands(command_2);
      // command_2.close();

      // Read input w-fields, iterate and output solution
      system.readWBasis("contents/omega/domainOn/omega_lam");
      system.iterate();
      system.writeWBasis("out/omega/domainOn/omega_lam");

      bool diff = true;
      for (int j = 0; j < ns; ++j) {
         for (int i = 0; i < nMonomer; ++i) {
           if ((std::abs(wFields_check[i][j] - system.wFields()[i][j]) > 1.0E-8)) {
               diff = false;
               std::cout <<"\n This is error for break:"<< 
                  (std::abs(wFields_check[i][j] - system.wFields()[i][j])) <<std::endl;
               std::cout <<"star index = "<< j << std::endl;
               break;
            }   
            else
               diff = true;
         }   
         if (diff==false) {
            break;
         }
      }   
      TEST_ASSERT(diff);
   }

   void testIterate2D_hex_rigid()
   {
      printMethod(TEST_FUNC);
      openLogFile("out/testIterate2D_hex_rigid.log"); 

      System<2> system;
      system.fileMaster().setInputPrefix(filePrefix());
      system.fileMaster().setOutputPrefix(filePrefix());

      std::ifstream in;
      openInputFile("in/domainOff/System2D", in); 
      system.readParam(in);
      in.close();

      //std::ifstream command;
      // openInputFile("in/domainOff/ReadOmega_hex", command);
      // system.readCommands(command);
      // command.close();

      // Read reference solution
      system.readWBasis("contents/omega/domainOff/omega_hex");

      int nMonomer = system.mixture().nMonomer();
      DArray<RField<2> > wFields_check;
      DArray<RField<2> > wFields;
      wFields_check.allocate(nMonomer);
      wFields.allocate(nMonomer);
      int ns = system.basis().nStar();
      for (int i = 0; i < nMonomer; ++i) {
          wFields_check[i].allocate(ns);
      }    

      for (int i = 0; i < nMonomer; ++i) {
         for (int j = 0; j < ns; ++j) {
            wFields_check[i][j] = system.wFields() [i] [j]; 
         }    
      }    

      // std::ifstream command_2;
      // openInputFile("in/domainOff/Iterate2d", command_2);
      // system.readCommands(command_2);
      // command_2.close();

      // Read initial guess, iterate, output solution
      system.readWBasis("contents/omega/domainOff/omega_hex");
      system.iterate();
      system.writeWBasis("out/omega/domainOff/omega_hex");

      // Compare current solution to reference solution
      bool diff = true;
      for (int j = 0; j < ns; ++j) {
         for (int i = 0; i < nMonomer; ++i) {
           //if ((std::abs(wFields_check[i][j] - system.wFields()[i][j]) >= 2.60828e-07)) {
           if ((std::abs(wFields_check[i][j] - system.wFields()[i][j]) >= 5.0e-07)) {
               // The above is the minimum error in the omega field.
               // Occurs for the first star            
               diff = false;
               std::cout << "\n This is error for break:" << 
                  (std::abs(wFields_check[i][j] - system.wFields()[i][j])) <<std::endl;
               std::cout <<"star index = "<< j << std::endl;
               break;
            }    
            else 
               diff = true;
         }    
         if (diff==false) {
            break;
         }    
      }    
      bool stress = false;
      if (std::abs(system.mixture().stress(0) - 0.010633960) < 1.0E-8) {
         // 0.010633960 is the stress calculated 
         // for this omega field for no stress relaxation using Fortran
         stress = true;
      }

      TEST_ASSERT(stress);
      TEST_ASSERT(diff);
   }

   void testIterate2D_hex_flex()
   {
      printMethod(TEST_FUNC);
      openLogFile("out/testIterate2D_hex_flex.log"); 

      System<2> system;
      system.fileMaster().setInputPrefix(filePrefix());
      system.fileMaster().setOutputPrefix(filePrefix());

      // Read parameter file
      std::ifstream in;
      openInputFile("in/domainOn/System2D", in);
      system.readParam(in);
      in.close();

      // std::ifstream command;
      // openInputFile("in/domainOn/ReadOmega_hex", command);
      // system.readCommands(command);
      // command.close();

      // Read reference solution (produced by Fortran code)
      system.readWBasis("contents/omega/domainOn/omega_hex");

      // Save reference solution to wFields_check array
      int nMonomer = system.mixture().nMonomer();
      DArray<RField<2> > wFields_check;
      DArray<RField<2> > wFields;
      wFields_check.allocate(nMonomer);
      wFields.allocate(nMonomer);
      int ns = system.basis().nStar();
      for (int i = 0; i < nMonomer; ++i) {
          wFields_check[i].allocate(ns);
      }

      for (int i = 0; i < nMonomer; ++i) {
         for (int j = 0; j < ns; ++j) {
            wFields_check[i][j] = system.wFields() [i] [j];
         }
      }

      // std::ifstream command_2;
      // openInputFile("in/domainOn/Iterate2d", command_2);
      // system.readCommands(command_2);
      // command_2.close();
      system.readWBasis("contents/omega/domainOn/omega_hex");
      system.iterate();
      system.writeWBasis("out/omega/domainOn/omega_hex");

      bool diff = true;
      for (int j = 0; j < ns; ++j) {
         for (int i = 0; i < nMonomer; ++i) {
            // if ((std::abs(wFields_check[i][j] - system.wFields()[i][j]) >= 2.58007e-07)) {
            if ((std::abs(wFields_check[i][j] - system.wFields()[i][j]) >= 5.0e-07)) {
               // The above is the maximum error in the omega field.
               // Occurs for the first star
               diff = false;
               std::cout <<"\n This is error for break:"<< 
                  (std::abs(wFields_check[i][j] - system.wFields()[i][j])) <<std::endl;
               std::cout <<"ns = "<< j << std::endl;
               break;
            } else {
               diff = true;
            }
         }
         if (diff==false) {
            break;
         }
      }
      TEST_ASSERT(diff);
   }

   void testIterate3D_bcc_rigid()
   {
      printMethod(TEST_FUNC);
      openLogFile("out/testIterate3D_bcc_rigid.log"); 

      System<3> system;
      system.fileMaster().setInputPrefix(filePrefix());
      system.fileMaster().setOutputPrefix(filePrefix());

      std::ifstream in;
      openInputFile("in/domainOff/System3D", in); 
      system.readParam(in);
      in.close();

      // std::ifstream command;
      // openInputFile("in/domainOff/ReadOmega_bcc", command);
      // system.readCommands(command);
      // command.close();
      system.readWBasis("contents/omega/domainOff/omega_bcc");

      int nMonomer = system.mixture().nMonomer();
      DArray<RField<3> > wFields_check;
      DArray<RField<3> > wFields;
      wFields_check.allocate(nMonomer);
      wFields.allocate(nMonomer);
      int ns = system.basis().nStar();
      for (int i = 0; i < nMonomer; ++i) {
          wFields_check[i].allocate(ns);
      }    

      for (int i = 0; i < nMonomer; ++i) {
         for (int j = 0; j < ns; ++j) {
            wFields_check[i][j] = system.wFields() [i] [j]; 
         }    
      }    

      // std::ifstream command_2;
      // openInputFile("in/domainOff/Iterate3d", command_2);
      // system.readCommands(command_2);
      // command_2.close();
      system.readWBasis("contents/omega/domainOff/omega_bcc");
      system.iterate();
      system.writeWBasis("out/omega/domainOff/omega_bcc");

      bool diff = true;
      for (int j = 0; j < ns; ++j) {
         for (int i = 0; i < nMonomer; ++i) {
           //if ((std::abs(wFields_check[i][j] - system.wFields()[i][j]) >= 1.02291e-07)) {
           if ((std::abs(wFields_check[i][j] - system.wFields()[i][j]) >= 5.0e-07)) {
               // The above is the maximum error in the omega field.
               // Occurs for the second star.               
               diff = false;
               std::cout <<"\n This is error for break:"<< 
                  (std::abs(wFields_check[i][j] - system.wFields()[i][j])) <<std::endl;
               std::cout <<"ns = "<< j << std::endl;
               break;
            }    
            else 
               diff = true;
         }
         if (diff==false) {
            break;
         }
      }
      bool stress = false;
      if (std::abs(system.mixture().stress(0) - 0.005242863) < 1.0E-8) {
         //0.005242863 is the stress calculated for this omega field 
         //for no stress relaxation using Fortran
         stress = true;
      }

      TEST_ASSERT(stress);
      TEST_ASSERT(diff);
   }

   void testIterate3D_bcc_flex()
   {
      printMethod(TEST_FUNC);
      openLogFile("out/testIterate3D_bcc_flex.log"); 

      System<3> system;
      system.fileMaster().setInputPrefix(filePrefix());
      system.fileMaster().setOutputPrefix(filePrefix());

      std::ifstream in; 
      openInputFile("in/domainOn/System3D", in);
      system.readParam(in);
      in.close();

      // std::ifstream command;
      // openInputFile("in/domainOn/ReadOmega_bcc", command);
      // system.readCommands(command);
      // command.close();
      system.readWBasis("contents/omega/domainOn/omega_bcc");

      int nMonomer = system.mixture().nMonomer();
      DArray<RField<3> > wFields_check;
      DArray<RField<3> > wFields;
      wFields_check.allocate(nMonomer);
      wFields.allocate(nMonomer);
      int ns = system.basis().nStar();
      for (int i = 0; i < nMonomer; ++i) {
          wFields_check[i].allocate(ns);
      }   

      for (int i = 0; i < nMonomer; ++i) {
         for (int j = 0; j < ns; ++j) {
            wFields_check[i][j] = system.wFields() [i] [j];
         }   
      }   

      // std::ifstream command_2;
      // openInputFile("in/domainOn/Iterate3d", command_2);
      // system.readCommands(command_2);
      // command_2.close();
      system.readWBasis("contents/omega/domainOn/omega_bcc");
      system.iterate();
      system.writeWBasis("out/omega/domainOn/omega_bcc");

      bool diff = true;
      for (int j = 0; j < ns; ++j) {
         for (int i = 0; i < nMonomer; ++i) {
           //if ((std::abs(wFields_check[i][j] - system.wFields()[i][j]) >=  1.09288e-07)) { 
           if ((std::abs(wFields_check[i][j] - system.wFields()[i][j]) >=  5.0e-07)) { 
               // The above is the maximum error in the omega field.
               // Occurs for the second star.
               diff = false;
               std::cout <<"\n This is error for break:"<< 
                  (std::abs(wFields_check[i][j] - system.wFields()[i][j])) <<std::endl;
               std::cout <<"ns = "<< j << std::endl;
               break;
            }
            else
               diff = true;
         }
         if (diff==false) {

            break;
         }
      }
      TEST_ASSERT(diff);
   }


};

TEST_BEGIN(SystemTest)
TEST_ADD(SystemTest, testConstructor1D)
TEST_ADD(SystemTest, testReadParameters1D)
TEST_ADD(SystemTest, testConversion1D_lam)
TEST_ADD(SystemTest, testConversion2D_hex)
TEST_ADD(SystemTest, testConversion3D_bcc)
TEST_ADD(SystemTest, testIterate1D_lam_rigid)
TEST_ADD(SystemTest, testIterate1D_lam_flex)
TEST_ADD(SystemTest, testIterate2D_hex_rigid)
TEST_ADD(SystemTest, testIterate2D_hex_flex)
TEST_ADD(SystemTest, testIterate3D_bcc_rigid)
TEST_ADD(SystemTest, testIterate3D_bcc_flex)

TEST_END(SystemTest)

#endif
