// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2008 Daniel Gomez Ferro <dgomezferro@gmail.com>
//
// Eigen is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// Alternatively, you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// Eigen is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License or the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License and a copy of the GNU General Public License along with
// Eigen. If not, see <http://www.gnu.org/licenses/>.

#include "sparse.h"

template<typename Scalar> void
initSPD(double density,
        Matrix<Scalar,Dynamic,Dynamic>& refMat,
        SparseMatrix<Scalar>& sparseMat)
{
  Matrix<Scalar,Dynamic,Dynamic> aux(refMat.rows(),refMat.cols());
  initSparse(density,refMat,sparseMat);
  refMat = refMat * refMat.adjoint();
  for (int k=0; k<2; ++k)
  {
    initSparse(density,aux,sparseMat,ForceNonZeroDiag);
    refMat += aux * aux.adjoint();
  }
  sparseMat.setZero();
  for (int j=0 ; j<sparseMat.cols(); ++j)
    for (int i=j ; i<sparseMat.rows(); ++i)
      if (refMat(i,j)!=Scalar(0))
        sparseMat.insert(i,j) = refMat(i,j);
  sparseMat.finalize();
}

template<typename Scalar> void sparse_solvers(int rows, int cols)
{
  double density = std::max(8./(rows*cols), 0.01);
  typedef Matrix<Scalar,Dynamic,Dynamic> DenseMatrix;
  typedef Matrix<Scalar,Dynamic,1> DenseVector;
  // Scalar eps = 1e-6;

  DenseVector vec1 = DenseVector::Random(rows);

  std::vector<Vector2i> zeroCoords;
  std::vector<Vector2i> nonzeroCoords;

  // test triangular solver
  {
    DenseVector vec2 = vec1, vec3 = vec1;
    SparseMatrix<Scalar> m2(rows, cols);
    DenseMatrix refMat2 = DenseMatrix::Zero(rows, cols);

    // lower - dense
    initSparse<Scalar>(density, refMat2, m2, ForceNonZeroDiag|MakeLowerTriangular, &zeroCoords, &nonzeroCoords);
    VERIFY_IS_APPROX(refMat2.template triangularView<LowerTriangular>().solve(vec2),
                     m2.template triangular<LowerTriangular>().solve(vec3));

    // upper - dense
    initSparse<Scalar>(density, refMat2, m2, ForceNonZeroDiag|MakeUpperTriangular, &zeroCoords, &nonzeroCoords);
    VERIFY_IS_APPROX(refMat2.template triangularView<UpperTriangular>().solve(vec2),
                     m2.template triangular<UpperTriangular>().solve(vec3));

    // TODO test row major

    SparseMatrix<Scalar> matB(rows, rows);
    DenseMatrix refMatB = DenseMatrix::Zero(rows, rows);

    // lower - sparse
    initSparse<Scalar>(density, refMat2, m2, ForceNonZeroDiag|MakeLowerTriangular);
    initSparse<Scalar>(density, refMatB, matB);
    refMat2.template triangularView<LowerTriangular>().solveInPlace(refMatB);
    m2.template triangular<LowerTriangular>().solveInPlace(matB);
    VERIFY_IS_APPROX(matB.toDense(), refMatB);

    // upper - sparse
    initSparse<Scalar>(density, refMat2, m2, ForceNonZeroDiag|MakeUpperTriangular);
    initSparse<Scalar>(density, refMatB, matB);
    refMat2.template triangularView<UpperTriangular>().solveInPlace(refMatB);
    m2.template triangular<UpperTriangular>().solveInPlace(matB);
    VERIFY_IS_APPROX(matB, refMatB);

    // test deprecated API
    initSparse<Scalar>(density, refMat2, m2, ForceNonZeroDiag|MakeLowerTriangular, &zeroCoords, &nonzeroCoords);
    VERIFY_IS_APPROX(refMat2.template triangularView<LowerTriangular>().solve(vec2),
                     m2.template triangular<LowerTriangular>().solve(vec3));
  }

  // test LLT
  {
    // TODO fix the issue with complex (see SparseLLT::solveInPlace)
    SparseMatrix<Scalar> m2(rows, cols);
    DenseMatrix refMat2(rows, cols);

    DenseVector b = DenseVector::Random(cols);
    DenseVector refX(cols), x(cols);

    initSPD(density, refMat2, m2);

    refMat2.llt().solve(b, &refX);
    typedef SparseMatrix<Scalar,LowerTriangular|SelfAdjoint> SparseSelfAdjointMatrix;
    if (!NumTraits<Scalar>::IsComplex)
    {
      x = b;
      SparseLLT<SparseSelfAdjointMatrix> (m2).solveInPlace(x);
      VERIFY(refX.isApprox(x,test_precision<Scalar>()) && "LLT: default");
    }
    #ifdef EIGEN_CHOLMOD_SUPPORT
    x = b;
    SparseLLT<SparseSelfAdjointMatrix,Cholmod>(m2).solveInPlace(x);
    VERIFY(refX.isApprox(x,test_precision<Scalar>()) && "LLT: cholmod");
    #endif

    #ifdef EIGEN_TAUCS_SUPPORT
    x = b;
    SparseLLT<SparseSelfAdjointMatrix,Taucs>(m2,IncompleteFactorization).solveInPlace(x);
    VERIFY(refX.isApprox(x,test_precision<Scalar>()) && "LLT: taucs (IncompleteFactorization)");
    // TODO fix TAUCS with complexes
    x = b;
    SparseLLT<SparseSelfAdjointMatrix,Taucs>(m2,SupernodalMultifrontal).solveInPlace(x);
    VERIFY(refX.isApprox(x,test_precision<Scalar>()) && "LLT: taucs (SupernodalMultifrontal)");
    x = b;
    SparseLLT<SparseSelfAdjointMatrix,Taucs>(m2,SupernodalLeftLooking).solveInPlace(x);
    VERIFY(refX.isApprox(x,test_precision<Scalar>()) && "LLT: taucs (SupernodalLeftLooking)");
    #endif
  }

  // test LDLT
  if (!NumTraits<Scalar>::IsComplex)
  {
    // TODO fix the issue with complex (see SparseLDLT::solveInPlace)
    SparseMatrix<Scalar> m2(rows, cols);
    DenseMatrix refMat2(rows, cols);

    DenseVector b = DenseVector::Random(cols);
    DenseVector refX(cols), x(cols);

    //initSPD(density, refMat2, m2);
    initSparse<Scalar>(density, refMat2, m2, ForceNonZeroDiag|MakeUpperTriangular, 0, 0);
    refMat2 += refMat2.adjoint();
    refMat2.diagonal() *= 0.5;

    refMat2.llt().solve(b, &refX); // FIXME use LLT to compute the reference because LDLT seems to fail with large matrices
    typedef SparseMatrix<Scalar,UpperTriangular|SelfAdjoint> SparseSelfAdjointMatrix;
    x = b;
    SparseLDLT<SparseSelfAdjointMatrix> ldlt(m2);
    if (ldlt.succeeded())
      ldlt.solveInPlace(x);
    VERIFY(refX.isApprox(x,test_precision<Scalar>()) && "LDLT: default");
  }

  // test LU
  {
    static int count = 0;
    SparseMatrix<Scalar> m2(rows, cols);
    DenseMatrix refMat2(rows, cols);

    DenseVector b = DenseVector::Random(cols);
    DenseVector refX(cols), x(cols);

    initSparse<Scalar>(density, refMat2, m2, ForceNonZeroDiag, &zeroCoords, &nonzeroCoords);

    FullPivLU<DenseMatrix> refLu(refMat2);
    refX = refLu.solve(b);
    #if defined(EIGEN_SUPERLU_SUPPORT) || defined(EIGEN_UMFPACK_SUPPORT)
    Scalar refDet = refLu.determinant();
    #endif
    x.setZero();
    // // SparseLU<SparseMatrix<Scalar> > (m2).solve(b,&x);
    // // VERIFY(refX.isApprox(x,test_precision<Scalar>()) && "LU: default");
    #ifdef EIGEN_SUPERLU_SUPPORT
    {
      x.setZero();
      SparseLU<SparseMatrix<Scalar>,SuperLU> slu(m2);
      if (slu.succeeded())
      {
        if (slu.solve(b,&x)) {
          VERIFY(refX.isApprox(x,test_precision<Scalar>()) && "LU: SuperLU");
        }
        // std::cerr << refDet << " == " << slu.determinant() << "\n";
        if (slu.solve(b, &x, SvTranspose)) {
          VERIFY(b.isApprox(m2.transpose() * x, test_precision<Scalar>()));
        }

        if (slu.solve(b, &x, SvAdjoint)) {
//          VERIFY(b.isApprox(m2.adjoint() * x, test_precision<Scalar>()));
        }

        if (count==0) {
          VERIFY_IS_APPROX(refDet,slu.determinant()); // FIXME det is not very stable for complex
        }
      }
    }
    #endif
    #ifdef EIGEN_UMFPACK_SUPPORT
    {
      // check solve
      x.setZero();
      SparseLU<SparseMatrix<Scalar>,UmfPack> slu(m2);
      if (slu.succeeded()) {
        if (slu.solve(b,&x)) {
          if (count==0) {
            VERIFY(refX.isApprox(x,test_precision<Scalar>()) && "LU: umfpack");  // FIXME solve is not very stable for complex
          }
        }
        VERIFY_IS_APPROX(refDet,slu.determinant());
        // TODO check the extracted data
        //std::cerr << slu.matrixL() << "\n";
      }
    }
    #endif
    count++;
  }

}

void test_sparse_solvers()
{
  for(int i = 0; i < g_repeat; i++) {
//     CALL_SUBTEST(sparse_solvers<double>(8, 8) );
    CALL_SUBTEST(sparse_solvers<std::complex<double> >(16, 16) );
//     CALL_SUBTEST(sparse_solvers<double>(100, 100) );
  }
}
