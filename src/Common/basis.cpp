#include <iostream>

#include <basis.hpp>
#include <vertex.hpp>

namespace HArDCore3D
{

  //------------------------------------------------------------------------------
  // Scalar monomial basis on a cell
  //------------------------------------------------------------------------------

  MonomialScalarBasisCell::MonomialScalarBasisCell(const Cell & T, size_t degree)
    : m_degree(degree),
      m_xT(T.center_mass()),
      m_hT(T.diam())
  {
    m_powers.reserve(dimension());
    for (size_t l = 0; l <= m_degree; l++) {
      for (size_t i = 0; i <= l; i++) {
        for (size_t j = 0; i + j <= l; j++) {
          m_powers.push_back(VectorZd(i, j, l-i-j));
        } // for j
      } // for i
    } // for l
  }

  MonomialScalarBasisCell::FunctionValue MonomialScalarBasisCell::function(size_t i, const VectorRd & x) const
  {
    VectorRd y = _coordinate_transform(x);
    const VectorZd & powers = m_powers[i];
    return std::pow(y(0), powers(0)) * std::pow(y(1), powers(1)) * std::pow(y(2), powers(2));
  }

  MonomialScalarBasisCell::GradientValue MonomialScalarBasisCell::gradient(size_t i, const VectorRd & x) const
  {
    VectorRd y = _coordinate_transform(x);
    const VectorZd & powers = m_powers[i];
    VectorRd grad = VectorRd::Zero();
    grad(0) = (powers(0) == 0 ? 0. : powers(0) * std::pow(y(0), powers(0)-1) * std::pow(y(1), powers(1)) * std::pow(y(2), powers(2)));
    grad(1) = (powers(1) == 0 ? 0. : std::pow(y(0), powers(0)) * powers(1) * std::pow(y(1), powers(1)-1) * std::pow(y(2), powers(2)));
    grad(2) = (powers(2) == 0 ? 0. : std::pow(y(0), powers(0)) * std::pow(y(1), powers(1)) * powers(2) * std::pow(y(2), powers(2)-1));
    return grad / m_hT;
  }  

  //------------------------------------------------------------------------------
  // Scalar monomial basis on a face
  //------------------------------------------------------------------------------

  MonomialScalarBasisFace::MonomialScalarBasisFace(const Face & F, size_t degree)
    : m_degree(degree),
      m_xF(F.center_mass()),
      m_hF(F.diam()),
      m_nF(F.normal()),
      m_jacobian(Eigen::Matrix<double, 2, dimspace>::Zero())
  {
    m_jacobian.row(0) = F.edge(0)->tangent();
    m_jacobian.row(1) = F.edge_normal(0);
    m_jacobian /= m_hF;
    
    // Generate powers
    m_powers.reserve(dimension());
    for (size_t l = 0; l <= m_degree; l++) {
      for (size_t i = 0; i <= l; i++) {
	m_powers.push_back(Eigen::Vector2i(i, l-i));
      } // for i
    } // for l
  }

  MonomialScalarBasisFace::FunctionValue MonomialScalarBasisFace::function(size_t i, const VectorRd & x) const
  {
    Eigen::Vector2d y = _coordinate_transform(x);
    const Eigen::Vector2i & powers = m_powers[i];
    return std::pow(y(0), powers(0)) * std::pow(y(1), powers(1));
  }

  MonomialScalarBasisFace::GradientValue MonomialScalarBasisFace::gradient(size_t i, const VectorRd & x) const
  {
    Eigen::Vector2d y = _coordinate_transform(x);
    const Eigen::Vector2i & powers = m_powers[i];
    Eigen::Vector2d grad = Eigen::Vector2d::Zero();
    grad(0) = (powers(0) == 0 ? 0. : powers(0) * std::pow(y(0), powers(0)-1) * std::pow(y(1), powers(1)));
    grad(1) = (powers(1) == 0 ? 0. : std::pow(y(0), powers(0)) * powers(1) * std::pow(y(1), powers(1)-1));
    return m_jacobian.transpose() * grad;
  }

  MonomialScalarBasisFace::GradientValue MonomialScalarBasisFace::curl(size_t i, const VectorRd & x) const
  {
    return gradient(i, x).cross(m_nF);
  }

  //------------------------------------------------------------------------------
  // Scalar monomial basis on an edge
  //------------------------------------------------------------------------------

  MonomialScalarBasisEdge::MonomialScalarBasisEdge(const Edge & E, size_t degree)
    : m_degree(degree),
      m_xE(E.center_mass()),
      m_hE(E.diam()),
      m_tE(E.tangent())
  {
    // Do nothing    
  }

  MonomialScalarBasisEdge::FunctionValue MonomialScalarBasisEdge::function(size_t i, const VectorRd & x) const
  {
    return std::pow(_coordinate_transform(x), i);
  }

  MonomialScalarBasisEdge::GradientValue MonomialScalarBasisEdge::gradient(size_t i, const VectorRd & x) const
  {
    return (i == 0 ? 0. : i * std::pow(_coordinate_transform(x), i - 1) / m_hE) * m_tE;
  }

  //------------------------------------------------------------------------------
  // A common notion of scalar product for scalars and vectors
  //------------------------------------------------------------------------------
  
  double scalar_product(const double & x, const double & y) {
    return x * y;
  }

  double scalar_product(const VectorRd & x, const VectorRd & y) {
    return x.dot(y);
  }

 
  boost::multi_array<double, 2>
  scalar_product(
		 const boost::multi_array<VectorRd, 2> & basis_quad,
		 const VectorRd & v
		 )
  {
    boost::multi_array<double, 2> basis_dot_v_quad( boost::extents[basis_quad.shape()[0]][basis_quad.shape()[1]] );
    std::transform(basis_quad.origin(), basis_quad.origin() + basis_quad.num_elements(),
		   basis_dot_v_quad.origin(), [&v](const VectorRd & x)->double { return x.dot(v); });
    return basis_dot_v_quad;
  }

  boost::multi_array<VectorRd, 2>
  vector_product(
		 const boost::multi_array<VectorRd, 2> & basis_quad,
		 const VectorRd & v
		 )
  {
    boost::multi_array<VectorRd, 2> basis_cross_v_quad( boost::extents[basis_quad.shape()[0]][basis_quad.shape()[1]] );
    std::transform(basis_quad.origin(), basis_quad.origin() + basis_quad.num_elements(),
		   basis_cross_v_quad.origin(), [&v](const VectorRd & x)->VectorRd { return x.cross(v); });
    return basis_cross_v_quad;
  }

  //------------------------------------------------------------------------------
  //      Gram matrices
  //------------------------------------------------------------------------------

  // Vector3d for B1, tensorialised double for B2
  Eigen::MatrixXd compute_gram_matrix(const boost::multi_array<VectorRd, 2> & B1,
				      const boost::multi_array<double, 2> & B2,
                                      const QuadratureRule & qr)
  {
    // Check that the basis evaluation and quadrature rule are coherent
    assert ( qr.size() == B1.shape()[1] && qr.size() == B2.shape()[1] );

    Eigen::MatrixXd M = Eigen::MatrixXd::Zero(B1.shape()[0], dimspace * B2.shape()[0]);
    for (size_t i = 0; i < B1.shape()[0]; i++) {
      for (size_t k = 0; k < dimspace; k++) {
        VectorRd ek = VectorRd::Zero();
        ek(k) = 1.;
        for(size_t j = 0; j < B2.shape()[0]; j++) {
          for (size_t iqn = 0; iqn < qr.size(); iqn++) {
            M(i,k * B2.shape()[0] + j) += qr[iqn].w * B1[i][iqn].dot(ek) * B2[j][iqn];
          } // for iqn
        } // for j
      } // for k
    } // for i
    return M;
  }

  // Gramm matrix for double-valued B1, B2
  Eigen::MatrixXd compute_gram_matrix(const boost::multi_array<double, 2> & B1,
				      const boost::multi_array<double, 2> & B2,
				      const QuadratureRule & qr,
				      const size_t nrows,
				      const size_t ncols,
				      const std::string sym
				      )
  {
    // Check that the basis evaluation and quadrature rule are coherent
    assert ( qr.size() == B1.shape()[1] && qr.size() == B2.shape()[1] );
    // Check that we don't ask for more members of family than available
    assert ( nrows <= B1.shape()[0] && ncols <= B2.shape()[0] );

    // Re-cast quadrature weights into ArrayXd to facilitate computations
    Eigen::ArrayXd qr_weights = Eigen::ArrayXd::Zero(qr.size());
    for (size_t iqn = 0; iqn < qr.size(); iqn++){
      qr_weights(iqn) = qr[iqn].w;
    }

    Eigen::MatrixXd M = Eigen::MatrixXd::Zero(nrows, ncols);
    for (size_t i = 0; i < nrows; i++) {
      size_t jcut = 0;
      if ( sym=="sym" ) jcut = i;
      for (size_t j = 0; j < jcut; j++){
        M(i, j) = M(j, i);
      }
      for(size_t j = jcut; j < ncols; j++) {
        std::vector<double> tmp(B1.shape()[1]);
        // Extract values at quadrature nodes for elements i of B1 and j of B2
        boost::multi_array<double, 1> B1i = B1[ boost::indices[i][boost::multi_array_types::index_range(0, B1.shape()[1])] ];
        boost::multi_array<double, 1> B2j = B2[ boost::indices[j][boost::multi_array_types::index_range(0, B1.shape()[1])] ];
        double *p1 = &B1i[0];
        double *p2 = &B2j[0];
        Eigen::ArrayXd B1i_as_array = Eigen::Map<Eigen::ArrayXd, Eigen::Unaligned>(p1, B1i.shape()[0]);
        Eigen::ArrayXd B2j_as_array = Eigen::Map<Eigen::ArrayXd, Eigen::Unaligned>(p2, B2j.shape()[0]);

        // Multiply by quadrature weights and sum (using .sum() of ArrayXd makes this step faster than a loop
        M(i,j) = (qr_weights * B1i_as_array * B2j_as_array).sum();
      } // for j
    } // for i
    return M;
  }

  // Gram matrix for double-valued complete family. Do not make this inline, this slows down calculations.
  Eigen::MatrixXd compute_gram_matrix(const boost::multi_array<double, 2> & B1,
				      const boost::multi_array<double, 2> & B2,
				      const QuadratureRule & qr,
				      const std::string sym
				      )
  {
    return compute_gram_matrix(B1, B2, qr, B1.shape()[0], B2.shape()[0], sym);
  }


  // Gram matrix for Vector3d-valued B1 and B2
  Eigen::MatrixXd compute_gram_matrix(const boost::multi_array<VectorRd, 2> & B1,
				      const boost::multi_array<VectorRd, 2> & B2,
				      const QuadratureRule & qr,
				      const size_t nrows,
				      const size_t ncols,
				      const std::string sym
				      )
  {
    // Check that the basis evaluation and quadrature rule are coherent
    assert ( qr.size() == B1.shape()[1] && qr.size() == B2.shape()[1] );
    // Check that we don't ask for more members of family than available
    assert ( nrows <= B1.shape()[0] && ncols <= B2.shape()[0] );

    // Re-cast quadrature weights into ArrayXd to make computations faster
    Eigen::ArrayXd qr_weights = Eigen::ArrayXd::Zero(qr.size());
    for (size_t iqn = 0; iqn < qr.size(); iqn++){
      qr_weights(iqn) = qr[iqn].w;
    }

    Eigen::MatrixXd M = Eigen::MatrixXd::Zero(nrows, ncols);
    for (size_t i = 0; i < nrows; i++) {
      size_t jcut = 0;
      if ( sym=="sym" ) jcut = i;
      for (size_t j = 0; j < jcut; j++){
        M(i, j) = M(j, i);
      }
      for(size_t j = jcut; j < ncols; j++) {
        // Array of scalar products
        Eigen::ArrayXd B1i_dot_B2j = Eigen::ArrayXd::Zero(qr.size());
        for (size_t iqn = 0; iqn < qr.size(); iqn++){
          B1i_dot_B2j(iqn) = B1[i][iqn].dot(B2[j][iqn]);
        }

        // Multiply component-wise by weights and sum
        M(i,j) = (qr_weights * B1i_dot_B2j).sum();
      } // for j
    } // for i
    return M;
  }

  // Gram matrix for Vector3d-valued complete family. Do not make this inline, this slows down actual calculations.
  Eigen::MatrixXd compute_gram_matrix(const boost::multi_array<VectorRd, 2> & B1, 
				      const boost::multi_array<VectorRd, 2> & B2, 
				      const QuadratureRule & qr,       
				      const std::string sym
				      )
    {
      return compute_gram_matrix(B1, B2, qr, B1.shape()[0], B2.shape()[0], sym);
    }


} // end of namespace HArDCore3D
