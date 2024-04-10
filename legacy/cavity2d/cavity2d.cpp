// cavity2d.cpp

// Lid-driven cavity flow 2d
// this is a benchmark for the freeLB library

// the top wall is set with a constant velocity,
// while the other walls are set with a no-slip boundary condition
// Bounce-Back-like method is used:
// Bounce-Back-Moving-Wall method for the top wall
// Bounce-Back method for the other walls

#include "freelb.h"
#include "freelb.hh"

// int Total_Macro_Step = 0;
using T = FLOAT;
using LatSet = D2Q9<T>;

/*----------------------------------------------
                Simulation Parameters
-----------------------------------------------*/
int Ni;
int Nj;
T Cell_Len;
T RT;
int Thread_Num;

/*physical property*/
T rho_ref;    // g/mm^3
T Dyna_Visc;  // Pa·s Dynamic viscosity of the liquid
T Kine_Visc;  // mm^2/s kinematic viscosity of the liquid
T Ra;         // Rayleigh number
/*init conditions*/
Vector<T, 2> U_Ini;  // mm/s
T U_Max;
T P_char;

/*bcs*/
Vector<T, 2> U_Wall;  // mm/s

// Simulation settings
int MaxStep;
int OutputStep;
T tol;
std::string work_dir;

void readParam() {
  /*reader*/
  iniReader param_reader("cavityparam2d.ini");
  // Thread_Num = param_reader.getValue<int>("OMP", "Thread_Num");
  /*mesh*/
  work_dir = param_reader.getValue<std::string>("workdir", "workdir_");
  // parallel
  Thread_Num = param_reader.getValue<int>("parallel", "thread_num");
  /*CA mesh*/
  Ni = param_reader.getValue<int>("Mesh", "Ni");
  Nj = param_reader.getValue<int>("Mesh", "Nj");
  Cell_Len = param_reader.getValue<T>("Mesh", "Cell_Len");
  /*physical property*/
  rho_ref = param_reader.getValue<T>("Physical_Property", "rho_ref");
  Dyna_Visc = param_reader.getValue<T>("Physical_Property", "Dyna_Visc");
  Kine_Visc = param_reader.getValue<T>("Physical_Property", "Kine_Visc");
  /*init conditions*/
  U_Ini[0] = param_reader.getValue<T>("Init_Conditions", "U_Ini0");
  U_Ini[1] = param_reader.getValue<T>("Init_Conditions", "U_Ini1");
  U_Max = param_reader.getValue<T>("Init_Conditions", "U_Max");
  P_char = param_reader.getValue<T>("Init_Conditions", "P_char");
  /*bcs*/
  U_Wall[0] = param_reader.getValue<T>("Boundary_Conditions", "Velo_Wall0");
  U_Wall[1] = param_reader.getValue<T>("Boundary_Conditions", "Velo_Wall1");
  // LB
  RT = param_reader.getValue<T>("LB", "RT");
  // Simulation settings
  MaxStep = param_reader.getValue<int>("Simulation_Settings", "TotalStep");
  OutputStep = param_reader.getValue<int>("Simulation_Settings", "OutputStep");
  tol = param_reader.getValue<T>("tolerance", "tol");

  /*output to console*/
  std::cout << "------------Simulation Parameters:-------------\n" << std::endl;
  std::cout << "[Simulation_Settings]:"
            << "TotalStep:         " << MaxStep << "\n"
            << "OutputStep:        " << OutputStep << "\n"
            << "Tolerance:         " << tol << "\n"
#ifdef _OPENMP
            << "Running on " << Thread_Num << " threads\n"
#endif
            << "----------------------------------------------\n"
            << std::endl;
}

int main() {
  Printer::Print_BigBanner(std::string("Initializing..."));

  readParam();

  // converters
  BaseConverter<T> BaseConv(LatSet::cs2);
  BaseConv.SimplifiedConvertFromViscosity(Ni, U_Max, Kine_Visc);
  // BaseConv.ConvertFromRT(Cell_Len, RT, rho_ref, Ni * Cell_Len, U_Max,
  // Kine_Visc);
  UnitConvManager<T> ConvManager(&BaseConv);
  ConvManager.Check_and_Print();

  // define geometry
  AABB<T, 2> cavity(Vector<T, 2>(T(0), T(0)),
                    Vector<T, 2>(T(Ni * Cell_Len), T(Nj * Cell_Len)));
  AABB<T, 2> toplid(Vector<T, 2>(Cell_Len, T(Nj * Cell_Len - 0.5 * Cell_Len)),
                    Vector<T, 2>(T((Ni - 1) * Cell_Len),
                                 T((Nj - 1) * Cell_Len + 0.5 * Cell_Len)));
  VoxelGeometry2D<T> Geo(Ni, Nj, cavity, Cell_Len);
  Geo.Setup<LatSet>();
  Geo.setFlag(toplid, 1, 2);
  Geo.WriteStruPoints();

  // velocity field
  VelocityField2D<T> Field(BaseConv, U_Ini, Geo);
  // set initial value of field
  Field.setVelocity(U_Wall, 2);

  // bcs
  GenericBounceBackLike<T, LatSet, BBlikemethod<T, LatSet>::normal_bounceback>
      NS_BB(1, "NS_BB");
  GenericBounceBackLike<T, LatSet,
                        BBlikemethod<T, LatSet>::movingwall_bounceback>
      NS_BBMW(2, "NS_BBMW");
  GenericBoundaryManager<T, LatSet> BM(&NS_BB, &NS_BBMW);

  // lbm method
  Genericlbm2D<T, LatSet> NS(Field, BaseConv, BM, "NS", "rho");
  NS.DefaultSetupIndex();
  NS.defaultSetupBCs();
  NS.EnableToleranceU();
  T res = 1;

  /*count and timer*/
  Timer MainLoopTimer;
  Timer OutputTimer;

  vtkWriterStruPoints<T, LatSet::d> vtkWriter("cavity2dVelocity", Geo.getVoxelSize(), Geo.getMin(),
                                              Ni, Nj);
  vtkWriter.addVectortoWriteList("velocity", &Field.getVelocity(),
                                 &Geo.getGlobalIdx());

  Printer::Print_BigBanner(std::string("Start Calculation..."));
  while (MainLoopTimer() < MaxStep && res > tol)
  // while (Total_Step < MaxStep)
  {
    ++MainLoopTimer;
    ++OutputTimer;
    // NS.Run<Equilibrium<T, LatSet>::Feq_secondOrder, true, true>();
    NS.Run<true, true>();
    if (MainLoopTimer() % OutputStep == 0) {
      res = NS.getToleranceU();
      OutputTimer.Print_InnerLoopPerformance(Ni * Nj, OutputStep);
      Printer::Print_Res<T>(res);
      // NS.WriteStruPoints(MainLoopTimer());
    }
  }
  vtkWriter.write();
  Printer::Print_BigBanner(std::string("Calculation Complete!"));
  MainLoopTimer.Print_MainLoopPerformance(Ni * Nj);

  return 0;
}