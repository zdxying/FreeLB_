/* This file is part of FreeLB
 *
 * Copyright (C) 2024 Yuan Man
 * E-mail contact: ymmanyuan@outlook.com
 * The most recent progress of FreeLB will be updated at
 * <https://github.com/zdxying/FreeLB>
 *
 * FreeLB is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * FreeLB is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with FreeLB. If
 * not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "freelb.h"
#include "freelb.hh"


using T = FLOAT;
using LatSet = D2Q9<T>;

/*----------------------------------------------
                Simulation Parameters
-----------------------------------------------*/
int Ni;
int Nj;
T Cell_Len;
int BlockNumX;
int BlockNum;

void readParam() {
  iniReader param_reader("refblock.ini");
  Ni = param_reader.getValue<int>("Mesh", "Ni");
  Nj = param_reader.getValue<int>("Mesh", "Nj");
  Cell_Len = param_reader.getValue<T>("Mesh", "Cell_Len");
  BlockNumX = param_reader.getValue<int>("Mesh", "BlockNumX");
  BlockNum = param_reader.getValue<int>("Mesh", "BlockNum");
}

int main(int argc, char* argv[]) {
  constexpr std::uint8_t VoidFlag = std::uint8_t(1);
  constexpr std::uint8_t AABBFlag = std::uint8_t(2);
  constexpr std::uint8_t BouncebackFlag = std::uint8_t(4);
  constexpr std::uint8_t BBMovingWallFlag = std::uint8_t(8);

  mpi().init(&argc, &argv);

  MPI_DEBUG_WAIT

  readParam();

  // ------------------ define geometry ------------------
  AABB<T, 2> cavity(Vector<T, 2>(T(0), T(0)),
                    Vector<T, 2>(T(Ni * Cell_Len), T(Nj * Cell_Len)));
  AABB<T, 2> toplid(Vector<T, 2>(Cell_Len, T((Nj - 1) * Cell_Len)),
                    Vector<T, 2>(T((Ni - 1) * Cell_Len), T(Nj * Cell_Len)));
  // test geocomm
  Circle<T> circle(T(Ni * Cell_Len / 8),
                   Vector<T, 2>(T(Ni * Cell_Len / 2), T(Nj * Cell_Len / 2)));
  // refblock

  AABB<T, 2> outercavity(
    Vector<T, 2>(T(Ni * Cell_Len) / BlockNumX, T(Nj * Cell_Len) / BlockNumX),
    Vector<T, 2>(T(Ni * Cell_Len) * (BlockNumX - 1) / BlockNumX,
                 T(Nj * Cell_Len) * (BlockNumX - 1) / BlockNumX));
  AABB<T, 2> innercavity(
    Vector<T, 2>(T(Ni * Cell_Len) * (BlockNumX / 2 - 1) / BlockNumX,
                 T(Nj * Cell_Len) * (BlockNumX / 2 - 1) / BlockNumX),
    Vector<T, 2>(T(Ni * Cell_Len) * (BlockNumX / 2 + 1) / BlockNumX,
                 T(Nj * Cell_Len) * (BlockNumX / 2 + 1) / BlockNumX));

  BlockGeometryHelper2D<T> GeoHelper(Ni, Nj, cavity, Cell_Len, Ni / BlockNumX);

  // GeoHelper.forEachBlockCell([&](BasicBlock<T, 2>& block) {
  //   if (!isOverlapped(block, outercavity)) {
  //     block.refine();
  //   }
  // });
  GeoHelper.forEachBlockCell([&](BasicBlock<T, 2>& block) {
    if (isOverlapped(block, innercavity)) {
      block.refine(std::uint8_t(2));
    }
  });
  GeoHelper.CheckRefine();
  GeoHelper.CreateBlocks();
  GeoHelper.AdaptiveOptimization(BlockNum);
  GeoHelper.LoadBalancing(mpi().getSize());

  // geometry
  BlockGeometry2D<T> Geo(GeoHelper);

  BlockFieldManager<RHO<T>, T, 2> FlagFM(Geo, VoidFlag);
  FlagFM.forEach([&](auto& field, std::size_t id) { field.SetField(id, mpi().getRank()); });
  // FlagFM.template SetupBoundary<LatSet>(cavity, BouncebackFlag);

  vtmo::ScalarWriter FlagWriter("flag", FlagFM);
  vtmo::vtmWriter<T, 2> GeoWriter("GeoFlag", Geo, 1);
  GeoWriter.addWriterSet(FlagWriter);
  GeoWriter.WriteBinary();
}