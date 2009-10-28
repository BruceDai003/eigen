// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2008 Gael Guennebaud <g.gael@free.fr>
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

#define EIGEN_NO_ASSERTION_CHECKING
#include "main.h"
#include <Eigen/Cholesky>
#include <Eigen/QR>

#ifdef HAS_GSL
#include "gsl_helper.h"
#endif

template<typename MatrixType> void cholesky(const MatrixType& m)
{
  /* this test covers the following files:
     LLT.h LDLT.h
  */
  int rows = m.rows();
  int cols = m.cols();

  typedef typename MatrixType::Scalar Scalar;
  typedef typename NumTraits<Scalar>::Real RealScalar;
  typedef Matrix<Scalar, MatrixType::RowsAtCompileTime, MatrixType::RowsAtCompileTime> SquareMatrixType;
  typedef Matrix<Scalar, MatrixType::RowsAtCompileTime, 1> VectorType;

  MatrixType a0 = MatrixType::Random(rows,cols);
  VectorType vecB = VectorType::Random(rows), vecX(rows);
  MatrixType matB = MatrixType::Random(rows,cols), matX(rows,cols);
  SquareMatrixType symm =  a0 * a0.adjoint();
  // let's make sure the matrix is not singular or near singular
  for (int k=0; k<3; ++k)
  {
    MatrixType a1 = MatrixType::Random(rows,cols);
    symm += a1 * a1.adjoint();
  }

  SquareMatrixType symmUp = symm.template triangularView<UpperTriangular>();
  SquareMatrixType symmLo = symm.template triangularView<LowerTriangular>();

  // to test if really Cholesky only uses the upper triangular part, uncomment the following
  // FIXME: currently that fails !!
  //symm.template part<StrictlyLowerTriangular>().setZero();

  #ifdef HAS_GSL
//   if (ei_is_same_type<RealScalar,double>::ret)
//   {
//     typedef GslTraits<Scalar> Gsl;
//     typename Gsl::Matrix gMatA=0, gSymm=0;
//     typename Gsl::Vector gVecB=0, gVecX=0;
//     convert<MatrixType>(symm, gSymm);
//     convert<MatrixType>(symm, gMatA);
//     convert<VectorType>(vecB, gVecB);
//     convert<VectorType>(vecB, gVecX);
//     Gsl::cholesky(gMatA);
//     Gsl::cholesky_solve(gMatA, gVecB, gVecX);
//     VectorType vecX(rows), _vecX, _vecB;
//     convert(gVecX, _vecX);
//     symm.llt().solve(vecB, &vecX);
//     Gsl::prod(gSymm, gVecX, gVecB);
//     convert(gVecB, _vecB);
//     // test gsl itself !
//     VERIFY_IS_APPROX(vecB, _vecB);
//     VERIFY_IS_APPROX(vecX, _vecX);
//
//     Gsl::free(gMatA);
//     Gsl::free(gSymm);
//     Gsl::free(gVecB);
//     Gsl::free(gVecX);
//   }
  #endif

  {
    LLT<SquareMatrixType,LowerTriangular> chollo(symmLo);
    VERIFY_IS_APPROX(symm, chollo.matrixL().toDense() * chollo.matrixL().adjoint().toDense());
    chollo.solve(vecB, &vecX);
    VERIFY_IS_APPROX(symm * vecX, vecB);
    chollo.solve(matB, &matX);
    VERIFY_IS_APPROX(symm * matX, matB);

    // test the upper mode
    LLT<SquareMatrixType,UpperTriangular> cholup(symmUp);
    VERIFY_IS_APPROX(symm, cholup.matrixL().toDense() * cholup.matrixL().adjoint().toDense());
    cholup.solve(vecB, &vecX);
    VERIFY_IS_APPROX(symm * vecX, vecB);
    cholup.solve(matB, &matX);
    VERIFY_IS_APPROX(symm * matX, matB);
  }

  int sign = ei_random<int>()%2 ? 1 : -1;

  if(sign == -1)
  {
    symm = -symm; // test a negative matrix
  }

  {
    LDLT<SquareMatrixType> ldlt(symm);
    // TODO(keir): This doesn't make sense now that LDLT pivots.
    //VERIFY_IS_APPROX(symm, ldlt.matrixL() * ldlt.vectorD().asDiagonal() * ldlt.matrixL().adjoint());
    ldlt.solve(vecB, &vecX);
    VERIFY_IS_APPROX(symm * vecX, vecB);
    ldlt.solve(matB, &matX);
    VERIFY_IS_APPROX(symm * matX, matB);
  }

}

template<typename MatrixType> void cholesky_verify_assert()
{
  MatrixType tmp;

  LLT<MatrixType> llt;
  VERIFY_RAISES_ASSERT(llt.matrixL())
  VERIFY_RAISES_ASSERT(llt.solve(tmp,&tmp))
  VERIFY_RAISES_ASSERT(llt.solveInPlace(&tmp))

  LDLT<MatrixType> ldlt;
  VERIFY_RAISES_ASSERT(ldlt.matrixL())
  VERIFY_RAISES_ASSERT(ldlt.permutationP())
  VERIFY_RAISES_ASSERT(ldlt.vectorD())
  VERIFY_RAISES_ASSERT(ldlt.isPositive())
  VERIFY_RAISES_ASSERT(ldlt.isNegative())
  VERIFY_RAISES_ASSERT(ldlt.solve(tmp,&tmp))
  VERIFY_RAISES_ASSERT(ldlt.solveInPlace(&tmp))
}

void test_cholesky()
{
  for(int i = 0; i < g_repeat; i++) {
    CALL_SUBTEST_1( cholesky(Matrix<double,1,1>()) );
    CALL_SUBTEST_2( cholesky(MatrixXd(1,1)) );
    CALL_SUBTEST_3( cholesky(Matrix2d()) );
    CALL_SUBTEST_4( cholesky(Matrix3f()) );
    CALL_SUBTEST_5( cholesky(Matrix4d()) );
    CALL_SUBTEST_2( cholesky(MatrixXd(200,200)) );
    CALL_SUBTEST_6( cholesky(MatrixXcd(100,100)) );
  }

  CALL_SUBTEST_4( cholesky_verify_assert<Matrix3f>() );
  CALL_SUBTEST_7( cholesky_verify_assert<Matrix3d>() );
  CALL_SUBTEST_8( cholesky_verify_assert<MatrixXf>() );
  CALL_SUBTEST_2( cholesky_verify_assert<MatrixXd>() );
}
