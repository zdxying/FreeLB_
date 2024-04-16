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

// physical field

#pragma once

// #include <array>
#include <algorithm>
#include <cstddef>
#include <vector>

#include "data_struct/Vector.h"
#include "utils/alias.h"
#include "utils/util.h"

// template <typename ArrayType>
// class AbstractGenericField {
//  public:
//   virtual ArrayType& getField(std::size_t i = 0) = 0;
//   virtual const ArrayType& getField(std::size_t i = 0) const = 0;
// };

// class GenericField final : public AbstractGenericField<ArrayType>

template <typename ArrayType, unsigned int D>
class GenericField {
 private:
  // field data
  std::array<ArrayType, D> _Data;
  // std::vector<ArrayType> _Data;

 public:
  GenericField() = delete;
  // std::vector<ArrayType> _Data:
  // it should be NOTED that DO NOT copy an ArrayType object if the copy
  // constructor of ArrayType is not defined.
  // both the original and the copy will point to the same data. When one of
  // them is destroyed, it will delete data, and then when the other is
  // destroyed, it will try to delete data again, causing a double delete error.
  // i.e., try use emplace_back(size) instead of push_back(ArrayType(size))
  // but push_back(ArrayType(size)) is ok if ArrayType has a copy constructor
  // std::array<ArrayType, D> _Data;
  // ArrayType must have a default constructor and a copy constructor
  // 23/12/12:
  // at this moment using std::vector<ArrayType> _Data is better
  // because std::vector<ArrayType> _Data will call the default constructor of
  // ArrayType, and then call the copy constructor of ArrayType to copy the
  // ArrayType(size, initialValue) to _Data[i]
  // 23/12/13:
  // using std::array<ArrayType, D> which is more comlex but avoid calling the
  // copy constructor of ArrayType
  // ---this is old version of 2 constructors
  // GenericField(std::size_t size) {
  //   for (unsigned int i = 0; i < D; ++i) _Data[i] = ArrayType(size);
  //   // _Data.emplace_back(size);
  // }
  // template <typename T>
  // GenericField(std::size_t size, T initialValue) {
  //   for (unsigned int i = 0; i < D; ++i)
  //     _Data[i] = ArrayType(size, initialValue);
  //   // _Data.emplace_back(size, initialValue);
  // }
  // ---end of old version

  GenericField(std::size_t size)
      : _Data(make_array<ArrayType, D>([&]() { return ArrayType(size); })) {}

  template <typename T>
  GenericField(std::size_t size, T initialValue)
      : _Data(make_array<ArrayType, D>([&]() { return ArrayType(size, initialValue); })) {
  }
  ~GenericField() = default;
  GenericField& operator=(const GenericField& genF) {
    if (&genF == this) return *this;
    for (unsigned int i = 0; i < D; ++i) _Data[i] = genF._Data[i];
    return *this;
  }

  ArrayType& getField(std::size_t i = 0) { return _Data[i]; }
  const ArrayType& getField(std::size_t i = 0) const { return _Data[i]; }
  // get<i>(id): return _Data[i][id];
  template <unsigned int i = 0>
  auto& get(std::size_t id) {
    return _Data[i][id];
  }
  template <unsigned int i = 0>
  const auto& get(std::size_t id) const {
    return _Data[i][id];
  }
  auto& get(std::size_t id, unsigned int dir) { return _Data[dir][id]; }
  const auto& get(std::size_t id, unsigned int dir) const { return _Data[dir][id]; }
  // get pointer to ith data in all arrays
  template <typename T>
  std::array<T*, D> getArray(std::size_t id) {
    std::array<T*, D> data{};
    for (unsigned int i = 0; i < D; ++i) data[i] = _Data[i].getdataPtr(id);
    return data;
  }
  // get all arrays
  template <typename T>
  std::array<T*, D> getArray() {
    std::array<T*, D> data{};
    for (unsigned int i = 0; i < D; ++i) data[i] = _Data[i].getdata();
    return data;
  }

  // you can directly call this function like: GeometryFlag.SetField(id, flag)
  // template <typename T, unsigned int i = 0> _Data[i].set(id, value);
  template <typename T, unsigned int i = 0>
  void SetField(std::size_t id, T value) {
    _Data[i].set(id, value);
  }
  template <typename T>
  void SetField(int i, std::size_t id, T value) {
    _Data[i].set(id, value);
  }
  // resize each array/field
  void Resize(std::size_t size) {
    for (unsigned int i = 0; i < D; ++i) _Data[i].Resize(size);
  }
  // init
  template <typename T>
  void Init(T value) {
    for (unsigned int i = 0; i < D; ++i) _Data[i].Init(value);
  }

  constexpr unsigned int Size() const { return D; }
};

// template <typename T>
// class AbstractArray {
//  public:
//   virtual const T& operator[](std::size_t i) const = 0;
//   virtual T& operator[](std::size_t i) = 0;
//   virtual std::size_t size() const = 0;
// };

template <typename T>
class GenericArray {
 private:
  // number of elements
  std::size_t count;
  // base pointer to the data
  T* data;

 public:
  GenericArray() = delete;
  // GenericArray() = delete;
  GenericArray(std::size_t size) : count(size), data(new T[size]{}) {
  std::fill(data, data + size, T{});
}

  GenericArray(std::size_t size, T InitValue) : count(size), data(new T[size]{}) {
    std::fill(data, data + size, InitValue);
  }
  // it is recommended to define a copy constructor in case of double free
  // copy constructor
  GenericArray(const GenericArray& arr) : count(arr.count), data(new T[arr.count]{}) {
    std::copy(arr.data, arr.data + count, data);
  }
  void Init(T InitValue) { std::fill(data, data + count, InitValue); }
  // Copy assignment operator
  GenericArray& operator=(const GenericArray& arr) {
    if (&arr == this) return *this;
    delete[] data;
    count = arr.count;
    data = new T[arr.count]{};
    std::copy(arr.data, arr.data + count, data);
    return *this;
  }
  void Resize(std::size_t size) {
    if (size == count) return;
    delete[] data;
    data = new T[size]{};
    count = size;
  }

  ~GenericArray() { delete[] data; }

  const T& operator[](std::size_t i) const { return data[i]; }
  T& operator[](std::size_t i) { return data[i]; }

  T* getdata() { return data; }
  const T* getdata() const { return data; }
  // return the pointer of ith element
  T* getdataPtr(std::size_t i) { return data + i; }
  const T* getdataPtr(std::size_t i) const { return data + i; }

  template <typename Func>
  void for_isflag(T flag, Func func) {
    for (std::size_t i = 0; i < count; ++i) {
      if (data[i] == flag) {
        func(i);
      }
    }
  }
  template <typename Func>
  void for_isNotflag(T flag, Func func) {
    for (std::size_t i = 0; i < count; ++i) {
      if (data[i] != flag) {
        func(i);
      }
    }
  }

  inline void set(std::size_t i, T value) { data[i] = value; }

  std::size_t size() const { return count; }
};

// Kummerländer A, Dorn M, Frank M, Krause MJ. Implicit propagation of directly
// addressed grids in lattice Boltzmann methods. Concurrency Computat Pract
// Exper. 2023;35(8):e7509. doi: 10.1002/cpe.7509 a cyclic array to present periodic
// shift(PS) pattern

template <typename T>
class CyclicArray {
 private:
  // number of elements
  std::size_t count;
  // base pointer to the data
  T* data;
  // shift
  std::ptrdiff_t shift;
  std::size_t remainder;
  std::array<T*, 2> start;
  // facilitate the access of data before the last shift(rotate)
  std::ptrdiff_t lastOffset;

 public:
  CyclicArray() = delete;
  // CyclicArray() : count(0), data(nullptr), shift(0), remainder(0), lastOffset(0) {}
  CyclicArray(std::size_t size)
      : count(size), data(new T[size]{}), shift(0), remainder(size), lastOffset(0) {
    std::fill(data, data + size, T{});
    refresh();
  }
  CyclicArray(std::size_t size, T InitValue)
      : count(size), data(new T[size]{}), shift(0), remainder(size), lastOffset(0) {
    std::fill(data, data + size, InitValue);
    refresh();
  }
  // copy constructor
  CyclicArray(const CyclicArray& arr)
      : count(arr.count), data(new T[arr.count]{}), shift(arr.shift),
        remainder(arr.remainder), lastOffset(arr.lastOffset) {
    std::copy(arr.data, arr.data + count, data);
    refresh();
  }
  void Init(T InitValue) { std::fill(data, data + count, InitValue); }
  // Copy assignment operator
  CyclicArray& operator=(const CyclicArray& arr) {
    if (&arr == this) return *this;
    delete[] data;
    count = arr.count;
    data = new T[arr.count]{};
    std::copy(arr.data, arr.data + count, data);
    shift = arr.shift;
    remainder = arr.remainder;
    lastOffset = arr.lastOffset;
    refresh();
    return *this;
  }
  void Resize(std::size_t size) {
    if (size == count) return;
    delete[] data;
    data = new T[size]{};
    count = size;
    shift = 0;
    remainder = size;
    lastOffset = 0;
    refresh();
  }

  ~CyclicArray() { delete[] data; }

  // get more info to achieve higher performance
  std::size_t getRemainder() const { return remainder; }
  const T* getStart(int i) const { return start[i]; }
  // end get more info

  const T& operator[](std::size_t i) const {
    return (i > remainder ? start[1] : start[0])[i];
  }
  T& operator[](std::size_t i) { return (i > remainder ? start[1] : start[0])[i]; }

  inline void set(std::size_t i, T value) {
    (i > remainder ? start[1] : start[0])[i] = value;
  }
  std::size_t size() const { return count; }
  T* getdata() { return data; }
  const T* getdata() const { return data; }
  // return the pointer of ith element
  T* getdataPtr(std::size_t i) {
    return i > remainder ? start[1] + i : start[0] + i;
    // return &((i > remainder ? start[1] : start[0])[i]);
  }
  const T* getdataPtr(std::size_t i) const {
    return i > remainder ? start[1] + i : start[0] + i;
    // return &((i > remainder ? start[1] : start[0])[i]);
  }

  // get data before the last shift(rotate), used in bcs
  T& getPrevious(std::size_t i) {
    std::ptrdiff_t prevIndex = i + lastOffset;
    if (prevIndex < 0) {
      prevIndex += count;
    } else if (prevIndex >= count) {
      prevIndex -= count;
    }
    return (prevIndex > remainder ? start[1] : start[0])[prevIndex];
  }

  void refresh() {
    const std::ptrdiff_t n = count;
    T* const base = data;
    if (shift >= 0) {
      remainder = n - shift - 1;
      start[0] = base + shift;
      start[1] = base - (n - shift);
    } else {
      remainder = -shift - 1;
      start[0] = base + (n + shift);
      start[1] = base + shift;
    }
  }

  void rotate(std::ptrdiff_t offset) {
    lastOffset = offset;
    const std::ptrdiff_t n = count;
    shift -= offset;
    if (shift >= n) {
      shift -= n;
    } else if (shift <= -n) {
      shift += n;
    }
    refresh();
  }
};

template <typename T, unsigned int D>
class BasicBlock;

// field data communication: interpolate from coarse to fine
template <template <typename> class ArrayType, typename datatype, typename FloatType,
          unsigned int D>
void FieldInterpolation2D(const GenericField<ArrayType<datatype>, D>& CField,
                          GenericField<ArrayType<datatype>, D>& FField,
                          const BasicBlock<FloatType, 2>& CBlock,
                          const BasicBlock<FloatType, 2>& CBaseBlock,
                          const BasicBlock<FloatType, 2>& FBlock,
                          const BasicBlock<FloatType, 2>& FBaseBlock) {
  // get intersection
  const AABB<FloatType, 2> intsec = getIntersection(CBaseBlock, FBaseBlock);
  int CNx = intsec.getExtension()[0] / CBlock.getVoxelSize();
  int CNy = intsec.getExtension()[1] / CBlock.getVoxelSize();
  // get start index of intsec in CBlock
  Vector<FloatType, 2> startC = intsec.getMin() - CBlock.getMin();
  int startCx_ = static_cast<int>(startC[0] / CBlock.getVoxelSize());
  int startCy_ = static_cast<int>(startC[1] / CBlock.getVoxelSize());
  // shift 1 voxel to left bottom for interpolation
  int startCx = startCx_ - 1;
  int startCy = startCy_ - 1;
  // start index of intsec in FBlock
  Vector<FloatType, 2> startF = intsec.getMin() - FBlock.getMin();
  int startFx = static_cast<int>(startF[0] / FBlock.getVoxelSize());
  int startFy = static_cast<int>(startF[1] / FBlock.getVoxelSize());
  // shift 1 voxel to right top for interpolation
  int startFx_ = startFx + 1;
  int startFy_ = startFy + 1;

  for (unsigned int iArr = 0; iArr < D; ++iArr) {
    const ArrayType<datatype>& CArray = CField.getField(iArr);
    ArrayType<datatype>& FArray = FField.getField(iArr);
    // interpolation
    // 0
    for (int iy = 0; iy < CNy; ++iy) {
      for (int ix = 0; ix < CNx; ++ix) {
        std::size_t Cid0 = (iy + startCy) * CBlock.getNx() + ix + startCx;
        std::size_t Cid1 = Cid0 + 1;
        std::size_t Cid2 = Cid0 + CBlock.getNx();
        std::size_t Cid3 = Cid2 + 1;
        std::size_t Fid = (iy * 2 + startFy) * FBlock.getNx() + ix * 2 + startFx;
        FArray[Fid] = CArray[Cid0] * FloatType(0.0625) +
                      CArray[Cid1] * FloatType(0.1875) +
                      CArray[Cid2] * FloatType(0.1875) + CArray[Cid3] * FloatType(0.5625);
      }
    }
    // 1
    for (int iy = 0; iy < CNy; ++iy) {
      for (int ix = 0; ix < CNx; ++ix) {
        std::size_t Cid0 = (iy + startCy) * CBlock.getNx() + ix + startCx_;
        std::size_t Cid1 = Cid0 + 1;
        std::size_t Cid2 = Cid0 + CBlock.getNx();
        std::size_t Cid3 = Cid2 + 1;
        std::size_t Fid = (iy * 2 + startFy) * FBlock.getNx() + ix * 2 + startFx_;
        FArray[Fid] = CArray[Cid0] * FloatType(0.1875) +
                      CArray[Cid1] * FloatType(0.0625) +
                      CArray[Cid2] * FloatType(0.5625) + CArray[Cid3] * FloatType(0.1875);
      }
    }
    // 2
    for (int iy = 0; iy < CNy; ++iy) {
      for (int ix = 0; ix < CNx; ++ix) {
        std::size_t Cid0 = (iy + startCy_) * CBlock.getNx() + ix + startCx;
        std::size_t Cid1 = Cid0 + 1;
        std::size_t Cid2 = Cid0 + CBlock.getNx();
        std::size_t Cid3 = Cid2 + 1;
        std::size_t Fid = (iy * 2 + startFy_) * FBlock.getNx() + ix * 2 + startFx;
        FArray[Fid] = CArray[Cid0] * FloatType(0.1875) +
                      CArray[Cid1] * FloatType(0.5625) +
                      CArray[Cid2] * FloatType(0.0625) + CArray[Cid3] * FloatType(0.1875);
      }
    }
    // 3
    for (int iy = 0; iy < CNy; ++iy) {
      for (int ix = 0; ix < CNx; ++ix) {
        std::size_t Cid0 = (iy + startCy_) * CBlock.getNx() + ix + startCx_;
        std::size_t Cid1 = Cid0 + 1;
        std::size_t Cid2 = Cid0 + CBlock.getNx();
        std::size_t Cid3 = Cid2 + 1;
        std::size_t Fid = (iy * 2 + startFy_) * FBlock.getNx() + ix * 2 + startFx_;
        FArray[Fid] = CArray[Cid0] * FloatType(0.5625) +
                      CArray[Cid1] * FloatType(0.1875) +
                      CArray[Cid2] * FloatType(0.1875) + CArray[Cid3] * FloatType(0.0625);
      }
    }
  }
}

// field data communication: average from fine to coarse
template <template <typename> class ArrayType, typename datatype, typename FloatType,
          unsigned int D>
void FieldAverage2D(const GenericField<ArrayType<datatype>, D>& FField,
                    GenericField<ArrayType<datatype>, D>& CField,
                    const BasicBlock<FloatType, 2>& FBlock,
                    const BasicBlock<FloatType, 2>& FBaseBlock,
                    const BasicBlock<FloatType, 2>& CBlock,
                    const BasicBlock<FloatType, 2>& CBaseBlock) {
  // get intersection
  const AABB<FloatType, 2> intsec = getIntersection(CBaseBlock, FBaseBlock);
  int CNx = intsec.getExtension()[0] / CBlock.getVoxelSize();
  int CNy = intsec.getExtension()[1] / CBlock.getVoxelSize();
  // get start index of intsec in CBlock
  Vector<FloatType, 2> startC = intsec.getMin() - CBlock.getMin();
  int startCx = static_cast<int>(startC[0] / CBlock.getVoxelSize());
  int startCy = static_cast<int>(startC[1] / CBlock.getVoxelSize());
  // start index of intsec in FBlock
  Vector<FloatType, 2> startF = intsec.getMin() - FBlock.getMin();
  int startFx = static_cast<int>(startF[0] / FBlock.getVoxelSize());
  int startFy = static_cast<int>(startF[1] / FBlock.getVoxelSize());

  for (unsigned int iArr = 0; iArr < D; ++iArr) {
    ArrayType<datatype>& CArray = CField.getField(iArr);
    const ArrayType<datatype>& FArray = FField.getField(iArr);

    for (int iy = 0; iy < CNy; ++iy) {
      for (int ix = 0; ix < CNx; ++ix) {
        std::size_t Cid = (iy + startCy) * CBlock.getNx() + ix + startCx;
        std::size_t Fid0 = (iy * 2 + startFy) * FBlock.getNx() + ix * 2 + startFx;
        std::size_t Fid1 = Fid0 + 1;
        std::size_t Fid2 = Fid0 + FBlock.getNx();
        std::size_t Fid3 = Fid2 + 1;
        CArray[Cid] =
          (FArray[Fid0] + FArray[Fid1] + FArray[Fid2] + FArray[Fid3]) * FloatType(0.25);
      }
    }
  }
}

// field data communication: simple copy
template <template <typename> class ArrayType, typename datatype, typename FloatType,
          unsigned int D>
void FieldCopy2D(const GenericField<ArrayType<datatype>, D>& FromField,
                 GenericField<ArrayType<datatype>, D>& ToField,
                 const BasicBlock<FloatType, 2>& FromBlock,
                 const BasicBlock<FloatType, 2>& FromBaseBlock,
                 const BasicBlock<FloatType, 2>& ToBlock,
                 const BasicBlock<FloatType, 2>& ToBaseBlock) {
  // get intersection
  const AABB<FloatType, 2> intsec = getIntersection(FromBaseBlock, ToBaseBlock);
  int Nx = intsec.getExtension()[0] / FromBlock.getVoxelSize();
  int Ny = intsec.getExtension()[1] / FromBlock.getVoxelSize();
  // get start index of intsec in FromBlock
  Vector<FloatType, 2> startFrom = intsec.getMin() - FromBlock.getMin();
  int startFromx = static_cast<int>(startFrom[0] / FromBlock.getVoxelSize());
  int startFromy = static_cast<int>(startFrom[1] / FromBlock.getVoxelSize());
  // start index of intsec in ToBlock
  Vector<FloatType, 2> startTo = intsec.getMin() - ToBlock.getMin();
  int startTox = static_cast<int>(startTo[0] / ToBlock.getVoxelSize());
  int startToy = static_cast<int>(startTo[1] / ToBlock.getVoxelSize());

  for (unsigned int iArr = 0; iArr < D; ++iArr) {
    ArrayType<datatype>& ToArray = ToField.getField(iArr);
    const ArrayType<datatype>& FromArray = FromField.getField(iArr);

    for (int iy = 0; iy < Ny; ++iy) {
      for (int ix = 0; ix < Nx; ++ix) {
        std::size_t Fromid = (iy + startFromy) * FromBlock.getNx() + ix + startFromx;
        std::size_t Toid = (iy + startToy) * ToBlock.getNx() + ix + startTox;
        ToArray[Toid] = FromArray[Fromid];
      }
    }
  }
}


// // field data communication: interpolate from coarse to fine cell
// template <template <typename> class ArrayType, typename T, unsigned int D>
// void InterpolationtoCell2D(GenericField<ArrayType<T>, D>& coarseF,
//                            GenericField<ArrayType<T>, D>& fineC,
//                            const BasicBlock<T, 2>& coarseFBlock,
//                            const BasicBlock<T, 2>& fineCBlock) {
//   for (unsigned int iArr = 0; iArr < D; ++iArr) {
//     ArrayType<T>& coarseFArray = coarseF.getField(iArr);
//     ArrayType<T>& fineCArray = fineC.getField(iArr);
//     Vector<T, 2> Shift_t = fineCBlock.getMin() - coarseFBlock.getMin();
//     // start index of coarse field
//     int xshift = static_cast<int>(Shift_t[0] / coarseFBlock.getVoxelSize()) - 1;
//     int yshift = static_cast<int>(Shift_t[1] / coarseFBlock.getVoxelSize()) - 1;
//     std::size_t shiftindex = yshift * coarseFBlock.getNx() + xshift;
//     // location of start point of coarse field
//     Vector<T, 2> coarseFStart =
//       coarseFBlock.getMinCenter() +
//       Vector<int, 2>{xshift, yshift} * coarseFBlock.getVoxelSize();
//     // location of start point of fine field
//     Vector<T, 2> fineCStart = fineCBlock.getMinCenter();
//     // interpolation
//     for (int iy = 0; iy < fineCBlock.getNy(); ++iy) {
//       for (int ix = 0; ix < fineCBlock.getNx(); ++ix) {
//         // location of fine cell
//         Vector<T, 2> fineCCell =
//           fineCStart + Vector<int, 2>{ix, iy} * fineCBlock.getVoxelSize();
//         // dist to coarse field start point
//         Vector<T, 2> dist = fineCCell - coarseFStart;
//         // left bottom coarse cell index
//         int x = static_cast<int>(dist[0] / coarseFBlock.getVoxelSize());
//         int y = static_cast<int>(dist[1] / coarseFBlock.getVoxelSize());
//         T remx = dist[0] - x * coarseFBlock.getVoxelSize();
//         T remy = dist[1] - y * coarseFBlock.getVoxelSize();
//         int scheme = 0;
//         if (remx < fineCBlock.getVoxelSize()) {
//           scheme += 1;
//         }
//         if (remy < fineCBlock.getVoxelSize()) {
//           scheme += 2;
//         }
//         /*
//         2 3
//         0 1
//         */
//         std::size_t Cid0 = y * coarseFBlock.getNx() + x + shiftindex;
//         std::size_t Cid1 = Cid0 + 1;
//         std::size_t Cid2 = Cid0 + coarseFBlock.getNx();
//         std::size_t Cid3 = Cid2 + 1;
//         std::size_t Fid = iy * fineCBlock.getNx() + ix;
//         // interpolate from coarse to fine
//         switch (scheme) {
//           case 0: {
//             fineCArray[Fid] =
//               coarseFArray[Cid0] * T(0.0625) + coarseFArray[Cid1] * T(0.1875) +
//               coarseFArray[Cid2] * T(0.1875) + coarseFArray[Cid3] * T(0.5625);
//           } break;
//           case 1: {
//             fineCArray[Fid] =
//               coarseFArray[Cid0] * T(0.1875) + coarseFArray[Cid1] * T(0.0625) +
//               coarseFArray[Cid2] * T(0.5625) + coarseFArray[Cid3] * T(0.1875);
//           } break;
//           case 2: {
//             fineCArray[Fid] =
//               coarseFArray[Cid0] * T(0.1875) + coarseFArray[Cid1] * T(0.5625) +
//               coarseFArray[Cid2] * T(0.0625) + coarseFArray[Cid3] * T(0.1875);
//           } break;
//           case 3: {
//             fineCArray[Fid] =
//               coarseFArray[Cid0] * T(0.5625) + coarseFArray[Cid1] * T(0.1875) +
//               coarseFArray[Cid2] * T(0.1875) + coarseFArray[Cid3] * T(0.0625);
//           } break;
//         }
//       }
//     }
//   }
// }

// // field data communication: average from fine to coarse cell
// template <template <typename> class ArrayType, typename T, unsigned int D>
// void AveragetoCell2D(GenericField<ArrayType<T>, D>& fineF,
//                      GenericField<ArrayType<T>, D>& CoarseC,
//                      const BasicBlock<T, 2>& fineFBlock,
//                      const BasicBlock<T, 2>& CoarseCBlock) {
//   for (unsigned int iArr = 0; iArr < D; ++iArr) {
//     ArrayType<T>& fineFArray = fineF.getField(iArr);
//     ArrayType<T>& CoarseCArray = CoarseC.getField(iArr);
//     Vector<T, 2> shift_t = CoarseCBlock.getMin() - fineFBlock.getMin();
//     // start index of fine field
//     int xshift = static_cast<int>(shift_t[0] / fineFBlock.getVoxelSize());
//     int yshift = static_cast<int>(shift_t[1] / fineFBlock.getVoxelSize());
//     std::size_t shiftindex = yshift * fineFBlock.getNx() + xshift;
//     // average
//     for (int iy = 0; iy < CoarseCBlock.getNy(); ++iy) {
//       for (int ix = 0; ix < CoarseCBlock.getNx(); ++ix) {
//         std::size_t Cid = iy * CoarseCBlock.getNx() + ix;
//         // left bottom corner fid
//         std::size_t Fid0 = (iy * 2) * fineFBlock.getNx() + ix * 2 + shiftindex;
//         // ids
//         std::size_t Fid1 = Fid0 + 1;
//         std::size_t Fid2 = Fid0 + fineFBlock.getNx();
//         std::size_t Fid3 = Fid2 + 1;
//         // average from fine to coarse
//         CoarseCArray[Cid] =
//           (fineFArray[Fid0] + fineFArray[Fid1] + fineFArray[Fid2] + fineFArray[Fid3]) *
//           T(0.25);
//       }
//     }
//   }
// }

// // field data communication: same level communication, simple copy to cell
// template <template <typename> class ArrayType, typename T, unsigned int D>
// void CopytoCell2D(GenericField<ArrayType<T>, D>& Field,
//                   GenericField<ArrayType<T>, D>& Cell, const BasicBlock<T, 2>&
//                   FieldBlock, const BasicBlock<T, 2>& CellBlock) {
//   for (unsigned int iArr = 0; iArr < D; ++iArr) {
//     ArrayType<T>& FieldArray = Field.getField(iArr);
//     ArrayType<T>& CellArray = Cell.getField(iArr);
//     Vector<T, 2> Shift_t = CellBlock.getMin() - FieldBlock.getMin();
//     // start index of cell
//     int xshift = static_cast<int>(Shift_t[0] / FieldBlock.getVoxelSize());
//     int yshift = static_cast<int>(Shift_t[1] / FieldBlock.getVoxelSize());
//     // copy
//     for (int iy = 0; iy < CellBlock.getNy(); ++iy) {
//       // start index of Cell
//       std::size_t CellStart = iy * CellBlock.getNx();
//       // start index of Field
//       std::size_t FieldStart = (iy + yshift) * FieldBlock.getNx() + xshift;
//       std::copy(FieldArray.getdataPtr(FieldStart),
//                 FieldArray.getdataPtr(FieldStart + CellBlock.getNx()),
//                 CellArray.getdataPtr(CellStart));
//     }
//   }
// }

// // field data communication: same level communication, simple copy to field
// template <template <typename> class ArrayType, typename T, unsigned int D>
// void CopytoField2D(GenericField<ArrayType<T>, D>& Field,
//                    GenericField<ArrayType<T>, D>& Cell,
//                    const BasicBlock<T, 2>& FieldBlock,
//                    const BasicBlock<T, 2>& CellBlock) {
//   for (unsigned int iArr = 0; iArr < D; ++iArr) {
//     ArrayType<T>& FieldArray = Field.getField(iArr);
//     ArrayType<T>& CellArray = Cell.getField(iArr);
//     Vector<T, 2> Shift_t = CellBlock.getMin() - FieldBlock.getMin();
//     // start index of cell
//     int xshift = static_cast<int>(Shift_t[0] / FieldBlock.getVoxelSize());
//     int yshift = static_cast<int>(Shift_t[1] / FieldBlock.getVoxelSize());
//     // copy
//     for (int iy = 0; iy < CellBlock.getNy(); ++iy) {
//       // start index of Cell
//       std::size_t CellStart = iy * CellBlock.getNx();
//       // start index of Field
//       std::size_t FieldStart = (iy + yshift) * FieldBlock.getNx() + xshift;
//       std::copy(CellArray.getdataPtr(CellStart),
//                 CellArray.getdataPtr(CellStart + CellBlock.getNx()),
//                 FieldArray.getdataPtr(FieldStart));
//     }
//   }
// }

// // field data communication: interpolate from coarse to fine; average from fine to
// coarse
// // only interpolate date withint box of overlap = 1
// // shift is used to adjust the interpolation point of coarse field
// template <template <typename> class ArrayType, typename T, unsigned int D>
// void FieldInterpolation2D(GenericField<ArrayType<T>, D>& coarseF, std::size_t newsize,
//                           const Vector<int, 2>& coarseMesh,
//                           const Vector<int, 2>& fineMesh, int shift = 0) {
//   for (unsigned int iArr = 0; iArr < D; ++iArr) {
//     ArrayType<T> bufferArray(newsize, T{});
//     ArrayType<T>& coarseArray = coarseF.getField(iArr);
//     for (int y = shift; y < coarseMesh[1] - shift; ++y) {
//       for (int x = shift; x < coarseMesh[0] - shift; ++x) {
//         // left bottom corner cid
//         std::size_t Cid0 = y * coarseMesh[0] + x;
//         // ids
//         std::size_t Cid1 = Cid0 + 1;
//         std::size_t Cid2 = Cid0 + coarseMesh[0];
//         std::size_t Cid3 = Cid2 + 1;

//         std::size_t Fid0 = (((y - shift) * 2) + 1) * fineMesh[0] + ((x - shift) * 2 +
//         1); std::size_t Fid1 = Fid0 + 1; std::size_t Fid2 = Fid0 + fineMesh[0];
//         std::size_t Fid3 = Fid2 + 1;

//         // interpolate from coarse to fine
//         // T(0.0625), T(0.1875), T(0.1875), T(0.5625)}
//         bufferArray[Fid0] = coarseArray[Cid0] * T(0.5625) +
//                             coarseArray[Cid1] * T(0.1875) +
//                             coarseArray[Cid2] * T(0.1875) + coarseArray[Cid3] *
//                             T(0.0625);
//         bufferArray[Fid1] = coarseArray[Cid0] * T(0.1875) +
//                             coarseArray[Cid1] * T(0.5625) +
//                             coarseArray[Cid2] * T(0.0625) + coarseArray[Cid3] *
//                             T(0.1875);
//         bufferArray[Fid2] = coarseArray[Cid0] * T(0.1875) +
//                             coarseArray[Cid1] * T(0.0625) +
//                             coarseArray[Cid2] * T(0.5625) + coarseArray[Cid3] *
//                             T(0.1875);
//         bufferArray[Fid3] = coarseArray[Cid0] * T(0.0625) +
//                             coarseArray[Cid1] * T(0.1875) +
//                             coarseArray[Cid2] * T(0.1875) + coarseArray[Cid3] *
//                             T(0.5625);
//       }
//     }
//     // update coarseArray
//     coarseArray = bufferArray;
//   }
// }

// // field data communication: average from fine to coarse
// template <template <typename> class ArrayType, typename T, unsigned int D>
// void FieldAverage2D(GenericField<ArrayType<T>, D>& fineF, std::size_t newsize,
//                     const Vector<int, 2>& coarseMesh, const Vector<int, 2>& fineMesh,
//                     int shift = 0) {
//   for (unsigned int iArr = 0; iArr < D; ++iArr) {
//     ArrayType<T> bufferArray(newsize, T{});
//     ArrayType<T>& fineArray = fineF.getField(iArr);
//     for (int y = shift; y < coarseMesh[1] - shift; ++y) {
//       for (int x = shift; x < coarseMesh[0] - shift; ++x) {
//         // cid
//         std::size_t Cid = y * coarseMesh[0] + x;
//         // left bottom corner fid
//         std::size_t Fid0 = ((y - shift) * 2) * fineMesh[0] + (x - shift) * 2;
//         // ids
//         std::size_t Fid1 = Fid0 + 1;
//         std::size_t Fid2 = Fid0 + fineMesh[0];
//         std::size_t Fid3 = Fid2 + 1;

//         // average from fine to coarse
//         bufferArray[Cid] =
//           (fineArray[Fid0] + fineArray[Fid1] + fineArray[Fid2] + fineArray[Fid3]) *
//           T(0.25);
//       }
//     }
//     // update fineArray
//     fineArray = bufferArray;
//   }
// }
