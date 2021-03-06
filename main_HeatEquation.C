// $Id$
//==============================================================================
//!
//! \file main_HeatEquation.C
//!
//! \date Jul 22 2015
//!
//! \author Arne Morten Kvarving / SINTEF
//!
//! \brief Main program for an isogeometric heat equation solver.
//!
//==============================================================================

#include "IFEM.h"
#include "AppCommon.h"
#include "SIM2D.h"
#include "SIM3D.h"
#include "SIMSolver.h"
#include "SIMHeatEquation.h"
#include "HDF5Writer.h"
#include "HeatEquation.h"
#include "XMLWriter.h"
#include "TimeIntUtils.h"
#include "Utilities.h"
#include "Profiler.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


//! \brief Setup and launch the simulation.
//! \param[in] infile The input file to process
//! \param[in] restartfile File to restart from. nullptr for no restart
//! \param[in] tit The time integration method to use. Either BE or BDF2
  template<class Dim>
int runSimulator(char* infile, char* restartfile, TimeIntegration::Method tIt)
{
  typedef SIMHeatEquation<Dim,HeatEquation> HeatSolver;

  HeatSolver            tempModel(TimeIntegration::Order(tIt));
  SIMSolver<HeatSolver> solver(tempModel);

  utl::profiler->start("Model input");
  IFEM::cout <<"\n\n0. Parsing input file(s)."
             <<"\n=========================\n";

  if (ConfigureSIM(tempModel, infile) || !solver.read(infile))
    return 1;

  utl::profiler->stop("Model input");

  tempModel.initSol();

  if (restartfile)
    SIM::handleRestart(tempModel, solver, restartfile, tempModel.getDumpInterval(),
                       TimeIntegration::Steps(tIt));

  DataExporter* exporter = nullptr;
  if (tempModel.opt.dumpHDF5(infile))
    exporter = SIM::handleDataOutput(tempModel, solver, tempModel.opt.hdf5,
                                     restartfile && tempModel.opt.hdf5 == restartfile,
                                     tempModel.getDumpInterval(),
                                     TimeIntegration::Steps(tIt));

  int res = solver.solveProblem(infile,exporter);

  tempModel.printFinalNorms(solver.getTimePrm());

  delete exporter;
  return res;
}


/*!
  \brief Main program for an isogeometric thermo-elastic solver.

  The input to the program is specified through the following
  command-line arguments. The arguments may be given in arbitrary order.

  \arg \a input-file : Input file with model definition
  \arg -dense :   Use the dense LAPACK matrix equation solver
  \arg -spr :     Use the SPR direct equation solver
  \arg -superlu : Use the sparse SuperLU equation solver
  \arg -samg :    Use the sparse algebraic multi-grid equation solver
  \arg -petsc :   Use equation solver from PETSc library
  \arg -lag : Use Lagrangian basis functions instead of splines/NURBS
  \arg -spec : Use Spectral basis functions instead of splines/NURBS
  \arg -LR : Use LR-spline basis functions instead of tensorial splines/NURBS
  \arg -nGauss \a n : Number of Gauss points over a knot-span in each direction
  \arg -vtf \a format : VTF-file format (-1=NONE, 0=ASCII, 1=BINARY)
  \arg -nviz \a nviz : Number of visualization points over each knot-span
  \arg -nu \a nu : Number of visualization points per knot-span in u-direction
  \arg -nv \a nv : Number of visualization points per knot-span in v-direction
  \arg -nw \a nw : Number of visualization points per knot-span in w-direction
  \arg -hdf5 : Write primary and projected secondary solution to HDF5 file
  \arg -2D : Use two-parametric simulation driver
*/

int main (int argc, char** argv)
{
  Profiler prof(argv[0]);
  utl::profiler->start("Initialization");

  bool twoD = false;
  char* infile = nullptr;
  char* restartfile = nullptr;
  TimeIntegration::Method tIt = TimeIntegration::BDF2;

  IFEM::Init(argc, argv);

  for (int i = 1; i < argc; i++)
    if (SIMoptions::ignoreOldOptions(argc,argv,i))
      ; // ignore the obsolete option
    else if (!strncmp(argv[i],"-2D",3))
      twoD = true;
    else if (!strncmp(argv[i],"-msg",4) && i < argc-1)
      SIMadmin::msgLevel = atoi(argv[++i]);
    else if (!strcmp(argv[i],"-be"))
      tIt = TimeIntegration::BE;
    else if (!strcmp(argv[i],"-bdf2"))
      tIt = TimeIntegration::BDF2;
    else if (!strcmp(argv[i],"-restart") && i < argc-1)
      restartfile = strtok(argv[++i],".");
    else if (!infile)
      infile = argv[i];
    else
      std::cerr <<"  ** Unknown option ignored: "<< argv[i] << std::endl;

  if (!infile)
  {
    std::cout <<"usage: "<< argv[0]
              <<" <inputfile> [-dense|-spr|-superlu[<nt>]|-samg|-petsc]\n"
              <<"       [-lag|-spec|-LR] [-2D[pstrain]] [-nGauss <n>]\n"
	      <<"       [-hdf5] [-vtf <format> [-nviz <nviz>]"
	      <<" [-nu <nu>] [-nv <nv>] [-nw <nw>]]\n";
    return 0;
  }

  std::cout <<"\n >>> IFEM Heat equation solver <<<"
            <<"\n =================================\n"
            <<"\n Executing command:\n";
  for (int i = 0; i < argc; i++) IFEM::cout <<" "<< argv[i];
  IFEM::cout <<"\n\nInput file: "<< infile;
  IFEM::getOptions().print(IFEM::cout) << std::endl;
  utl::profiler->stop("Initialization");

  if (twoD)
    return runSimulator<SIM2D>(infile, restartfile, tIt);
  else
    return runSimulator<SIM3D>(infile, restartfile, tIt);
}
