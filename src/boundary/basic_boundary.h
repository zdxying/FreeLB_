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

// basic_boundary.h

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>

#include "data_struct/block_lattice.h"
#include "utils/alias.h"


// fixed boundary cell structure
struct FixedBdCell {
  // outflow directions
  std::vector<unsigned int> outflows;
  // cell id
  std::size_t Id;

  FixedBdCell(std::size_t id, int q) : Id(id) { outflows.reserve(q); }
};

class AbstractBoundary {
 public:
  virtual void Apply() = 0;
  virtual void getinfo() {}
  virtual void UpdateRho() {}
  virtual void UpdateU() {}
  // virtual void UpdateU_F() = 0;
};

// flagType should be std::uint8_t or enum of std::uint8_t
template <typename T, typename LatSet, typename flagType>
class FixedBoundary : public AbstractBoundary {
 protected:
  // boundary cell
  std::vector<FixedBdCell> BdCells;
  // reference to lattice
  PopLattice<T, LatSet> &Lat;
  // reference to geometry
  Geometry<T, LatSet::d> &Geo;
  // geometry flag
  GenericArray<flagType> &Field;
  // boundary cell flag
  std::uint8_t BdCellFlag;
  // boundary flag
  std::uint8_t voidFlag;

 public:
  FixedBoundary(PopLattice<T, LatSet> &lat, std::uint8_t cellflag, std::uint8_t voidflag)
      : Lat(lat), Geo(lat.getGeo()), Field(lat.getGeo().getGeoFlagField().getField()),
        BdCellFlag(cellflag), voidFlag(voidflag) {
    Setup();
  }
  FixedBoundary(PopLattice<T, LatSet> &lat, GenericArray<flagType> &f,
                std::uint8_t cellflag, std::uint8_t voidflag)
      : Lat(lat), Geo(lat.getGeo()), Field(f), BdCellFlag(cellflag), voidFlag(voidflag) {
    Setup();
  }
  // get boundary cell flag
  std::uint8_t getBdCellFlag() const { return BdCellFlag; }
  // get void cell flag
  std::uint8_t getVoidFlag() const { return voidFlag; }
  // add to boundary cells
  void addtoBd(std::size_t id) {
    BdCells.emplace_back(id, LatSet::q);
    // get reference to the last element
    FixedBdCell &fixedbdcell = BdCells.back();
    // get neighbor
    // Attention: if voidFlag is 0, DO NOT use util::isFlag
    for (unsigned int k = 1; k < LatSet::q; ++k) {
      // if (Field[Lat.getNbrId(id, k)] == voidFlag &&
      //     Field[Lat.getNbrId(id, latset::opp<LatSet>(k))] != voidFlag) {
      //   fixedbdcell.outflows.push_back(latset::opp<LatSet>(k));
      // }

      // using util::isFlag requires voidFlag to be non-zero
      if (util::isFlag(Field[Lat.getNbrId(id, k)], voidFlag)) {
        fixedbdcell.outflows.push_back(latset::opp<LatSet>(k));
      }
    }
  }
  // setup boundary cells
  void Setup() {
    for (int id = 0; id < Geo.getVoxelsNum(); ++id) {
      if (util::isFlag(Field[id], BdCellFlag)) addtoBd(id);
    }
  }
};

template <typename T, typename LatSet, typename flagType>
class MovingBoundary : public AbstractBoundary {
 protected:
  // reference to lattice
  PopLattice<T, LatSet> &Lat;
  // reference to geometry
  Geometry<T, LatSet::d> &Geo;
  // boundary cells
  std::vector<std::size_t> &Ids;
  // geometry flag
  GenericArray<flagType> &Field;
  // boundary cell flag
  std::uint8_t BdCellFlag;
  // boundary flag
  std::uint8_t voidFlag;


 public:
  MovingBoundary(PopLattice<T, LatSet> &lat, std::vector<std::size_t> &ids,
                 std::uint8_t voidflag, std::uint8_t cellflag)
      : Lat(lat), Geo(lat.getGeo()), Ids(ids),
        Field(lat.getGeo().getGeoFlagField().getField()), BdCellFlag(cellflag),
        voidFlag(voidflag) {}
  MovingBoundary(PopLattice<T, LatSet> &lat, std::vector<std::size_t> &ids,
                 GenericArray<flagType> &f, std::uint8_t voidflag, std::uint8_t cellflag)
      : Lat(lat), Geo(lat.getGeo()), Ids(ids), Field(f), BdCellFlag(cellflag),
        voidFlag(voidflag) {}

  // get boundary cell flag
  std::uint8_t getBdCellFlag() const { return BdCellFlag; }
  // get void cell flag
  std::uint8_t getVoidFlag() const { return voidFlag; }
  // get std::vector<std::size_t> &Ids;
  std::vector<std::size_t> &getIds() { return Ids; }
  // update boundary cells: std::vector<std::size_t> &Ids;
  void UpdateBdCells() {
    Ids.clear();
    for (std::size_t id = 0; id < Geo.getVoxelsNum(); ++id) {
      if (util::isFlag(Field[id], BdCellFlag)) Ids.push_back(id);
    }
  }
};

// --------------------------------------------------------------------------------------
// -----------------------------------BlockBoundary--------------------------------------
// --------------------------------------------------------------------------------------

class AbstractBlockBoundary {
 public:
  virtual void Apply(std::int64_t count) = 0;
  virtual void Apply() = 0;
};

template <typename BLOCKLATTICE, typename ArrayType>
class BlockFixedBoundary {
 protected:
  // boundary cell : legacy
  std::vector<FixedBdCell> BdCells;
  // new scheme
  // boundary blocks with outflow directions: vector<pair<vec_of_flowdirs, vec_of_cellids>>
  std::vector<std::pair<std::vector<unsigned int>, std::vector<std::size_t>>> Cells;

  // reference to lattice
  BLOCKLATTICE &Lat;
  // geometry flag
  const ArrayType &Field;
  // boundary cell flag
  std::uint8_t BdCellFlag;
  // boundary flag
  std::uint8_t voidFlag;

 public:
  using LatSet = typename BLOCKLATTICE::LatticeSet;

  BlockFixedBoundary(BLOCKLATTICE &lat, const ArrayType &f, std::uint8_t cellflag,
                     std::uint8_t voidflag)
      : Lat(lat), Field(f), BdCellFlag(cellflag), voidFlag(voidflag) {
    Setup();
  }
  // get boundary cell flag
  std::uint8_t getBdCellFlag() const { return BdCellFlag; }
  // get void cell flag
  std::uint8_t getVoidFlag() const { return voidFlag; }
  BLOCKLATTICE &getLat() { return Lat; }
  // add to boundary cells: std::vector<FixedBdCell> BdCells
  void addtoBd(std::size_t id) {
    BdCells.emplace_back(id, LatSet::q);
    // get reference to the last element
    FixedBdCell &fixedbdcell = BdCells.back();
    // get neighbor
    // Attention: if voidFlag is 0, DO NOT use util::isFlag
    for (unsigned int k = 1; k < LatSet::q; ++k) {
      // if (util::isFlag(Field[Lat.getNbrId(id, k)], voidFlag) &&
      //     !util::isFlag(Field[Lat.getNbrId(id, latset::opp<LatSet>(k))], voidFlag)) {
      // using util::isFlag requires voidFlag to be non-zero
      if (util::isFlag(Field[Lat.getNbrId(id, k)], voidFlag)) {
        fixedbdcell.outflows.push_back(latset::opp<LatSet>(k));
      }
    }
  }
  // 
  void addtoCells(std::size_t id) {
    // find all outflow directions
    std::vector<unsigned int> outflows;
    for (unsigned int k = 1; k < LatSet::q; ++k) {
      if (util::isFlag(Field[Lat.getNbrId(id, k)], voidFlag)) {
        outflows.push_back(latset::opp<LatSet>(k));
      }
    }
    // search for existing outflow directions
    for (auto& pair : Cells) {
      // using operator== to compare vectors
      if (pair.first == outflows) {
        // already exists: add cell index
        pair.second.push_back(id);
        return;
      }
    }
    // not found: add new outflow directions
    Cells.emplace_back(std::make_pair(outflows, std::vector<std::size_t>{id}));
  }
  // setup boundary cells
  void Setup() {
    // std::size_t reserveSize{};
    // if constexpr (LatSet::d == 2) {
    //   reserveSize = (Lat.getNx() + Lat.getNy()) * 2;
    // } else if constexpr (LatSet::d == 3) {
    //   reserveSize = (Lat.getNx() * Lat.getNy() + Lat.getNx() * Lat.getNz() +
    //                  Lat.getNy() * Lat.getNz()) * 2;
    // }
    // Cells.reserve(reserveSize);
    // add inner cells
    if constexpr (LatSet::d == 2) {
      for (int iy = Lat.getOverlap(); iy < Lat.getNy() - Lat.getOverlap(); ++iy) {
        for (int ix = Lat.getOverlap(); ix < Lat.getNx() - Lat.getOverlap(); ++ix) {
          std::size_t id = ix + iy * Lat.getNx();
          if (util::isFlag(Field[id], BdCellFlag)) addtoBd(id);
        }
      }
    } else if constexpr (LatSet::d == 3) {
      for (int iz = Lat.getOverlap(); iz < Lat.getNz() - Lat.getOverlap(); ++iz) {
        for (int iy = Lat.getOverlap(); iy < Lat.getNy() - Lat.getOverlap(); ++iy) {
          for (int ix = Lat.getOverlap(); ix < Lat.getNx() - Lat.getOverlap(); ++ix) {
            std::size_t id = ix + iy * Lat.getProjection()[1] + iz * Lat.getProjection()[2];
            if (util::isFlag(Field[id], BdCellFlag)) addtoBd(id);
          }
        }
      }
    }
    // shrink capacity to actual size
    // Cells.shrink_to_fit();
  }
};

template <typename BLOCKLATTICE, typename ArrayType>
class BlockMovingBoundary {
 protected:
  // boundary cells
  std::vector<std::size_t> &Ids;
  // reference to lattice
  BLOCKLATTICE &Lat;
  // geometry flag
  ArrayType &Field;
  // boundary cell flag
  std::uint8_t BdCellFlag;
  // boundary flag
  std::uint8_t voidFlag;


 public:
  BlockMovingBoundary(BLOCKLATTICE &lat, std::vector<std::size_t> &ids, ArrayType &f,
                      std::uint8_t voidflag, std::uint8_t cellflag)
      : Ids(ids), Lat(lat), Field(f), BdCellFlag(cellflag), voidFlag(voidflag) {}
  // get boundary cell flag
  std::uint8_t getBdCellFlag() const { return BdCellFlag; }
  // get void cell flag
  std::uint8_t getVoidFlag() const { return voidFlag; }
  // get boundary cells std::vector<std::size_t> &Ids;
  std::vector<std::size_t> &getIds() { return Ids; }
  BLOCKLATTICE &getLat() { return Lat; }
  // update boundary cells
  void UpdateBdCells() {
    Ids.clear();
    for (std::size_t id = 0; id < Lat.getBlock().getN(); ++id) {
      if (util::isFlag(Field[id], BdCellFlag)) Ids.push_back(id);
    }
  }
};

class BoundaryManager {
 private:
  std::vector<AbstractBoundary *> _Boundaries;

 public:
  BoundaryManager(std::vector<AbstractBoundary *> boundaries) : _Boundaries(boundaries) {
    printinfo();
  }
  template <typename... Args>
  BoundaryManager(Args *...args) : _Boundaries{args...} {
    printinfo();
  }

  void Apply() {
    for (AbstractBoundary *boundary : _Boundaries) boundary->Apply();
  }
  void printinfo() {
    std::cout << "[Boundary Statistics]: " << "\n"
              << "Boundary Type  |  Number of Boundary Cells" << std::endl;
    for (AbstractBoundary *boundary : _Boundaries) boundary->getinfo();
  }
  void UpdateRho() {
    for (AbstractBoundary *boundary : _Boundaries) boundary->UpdateRho();
  }
  void UpdateU() {
    for (AbstractBoundary *boundary : _Boundaries) boundary->UpdateU();
  }
};

class BlockBoundaryManager {
 private:
  std::vector<AbstractBlockBoundary *> _Boundaries;

 public:
  BlockBoundaryManager(std::vector<AbstractBlockBoundary *> boundaries)
      : _Boundaries(boundaries) {}

  template <typename... Args>
  BlockBoundaryManager(Args *...args) : _Boundaries{args...} {}

  void Apply(std::int64_t count) {
    for (AbstractBlockBoundary *boundary : _Boundaries) boundary->Apply(count);
  }
  void Apply() {
    for (AbstractBlockBoundary *boundary : _Boundaries) boundary->Apply();
  }
};

// non-local boundary
template <typename BLOCKLATTICE, typename ArrayType>
class NonLocalBoundary {
 public:
  using LatSet = typename BLOCKLATTICE::LatticeSet;
  using T = typename LatSet::FloatType;
  static constexpr unsigned int D = LatSet::d;
  using TypePack = typename BLOCKLATTICE::FieldTypePack;

 protected:
  // boundary cell
  std::vector<std::size_t> BdCells;
  // reference to lattice
  BLOCKLATTICE &Lat;
  // geometry flag
  const ArrayType &Field;
  // boundary cell flag
  std::uint8_t BdCellFlag;
  // boundary flag
  std::uint8_t voidFlag;
  // base communication structure
  // it is NOT the same comm set in BlockGeometry
  BasicCommSet<T, D> BaseCommSet;
  // LatticeComm for Pop communication
  LatticeCommSet<LatSet, TypePack> LatticeComm;

 public:

  NonLocalBoundary(BLOCKLATTICE &lat, const ArrayType &f, std::uint8_t cellflag,
                   std::uint8_t voidflag)
      : Lat(lat), Field(f), BdCellFlag(cellflag), voidFlag(voidflag) {
        Setup();
      }
  // get boundary cell flag
  std::uint8_t getBdCellFlag() const { return BdCellFlag; }
  // get void cell flag
  std::uint8_t getVoidFlag() const { return voidFlag; }
  BLOCKLATTICE &getLat() { return Lat; }
  // BasicCommSet<T, D> BaseCommSet
  BasicCommSet<T, D> &getBaseCommSet() { return BaseCommSet; }
  // LatticeComm<T, TypePack> LatticeComm
  LatticeCommSet<LatSet, TypePack> &getLatticeComm() { return LatticeComm; }
  // std::vector<std::size_t> BdCells;
  std::vector<std::size_t> &getBdCells() { return BdCells; }
  // setup boundary cells
  void Setup() {
    std::size_t reserveSize;
    if constexpr (D == 2) {
      reserveSize = (Lat.getNx() + Lat.getNy()) * 2;
    } else if constexpr (D == 3) {
      reserveSize = (Lat.getNx() * Lat.getNy() + Lat.getNx() * Lat.getNz() +
                     Lat.getNy() * Lat.getNz()) * 2;
    }
    BdCells.reserve(reserveSize);
    if constexpr (D == 2) {
      for (int iy = 0; iy < Lat.getNy() ; ++iy) {
        for (int ix = 0; ix < Lat.getNx(); ++ix) {
          std::size_t id = ix + iy * Lat.getNx();
          if (util::isFlag(Field[id], BdCellFlag)) BdCells.emplace_back(id);
        }
      }
    } else if constexpr (D == 3) {
      for (int iz = 0; iz < Lat.getNz(); ++iz) {
        for (int iy = 0; iy < Lat.getNy(); ++iy) {
          for (int ix = 0; ix < Lat.getNx(); ++ix) {
            std::size_t id = ix + iy * Lat.getProjection()[1] + iz * Lat.getProjection()[2];
            if (util::isFlag(Field[id], BdCellFlag)) BdCells.emplace_back(id);
          }
        }
      }
    }
    // shrink capacity to actual size
    BdCells.shrink_to_fit();
  }
};