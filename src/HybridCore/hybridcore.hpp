// Core data structures and methods required to implement hybrid schemes in 3D (polynomial unknowns
// in the cells and on the faces, such as Hybrid High-order (HHO) schemes).
//
// Provides:
//  - Hybrid polynomial basis functions (on the cells and faces of the mesh)
//  - Generic routines to create quadrature nodes over cells and faces of the mesh
//  - Interpolation of general functions onto the HHO space
//  - Methods for integrating, evaluating, and computing norms of HHO solutions
//
// Author: Jerome Droniou (jerome.droniou@monash.edu)
//

/*
 *
 *	This library was developed around HHO methods, although some parts of it have a more
 * general purpose. If you use this code or part of it in a scientific publication, 
 * please mention the following book as a reference for the underlying principles
 * of HHO schemes:
 *
 * The Hybrid High-Order Method for Polytopal Meshes: Design, Analysis, and Applications.
 * D. A. Di Pietro and J. Droniou. 2019, 516p. 
 * url: https://hal.archives-ouvertes.fr/hal-02151813.
 *
 */

#ifndef HYBRIDCORE_HPP
#define HYBRIDCORE_HPP

#include <cassert>
#include <cmath>

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <mesh.hpp>
#include <cell.hpp>
#include <face.hpp>
#include <quadraturerule.hpp>

/*!	
 * @defgroup HybridCore 
 * @brief Classes providing tools to implement schemes having polynomial unknowns on the faces and in the cells
 * The basic ordering of vectors is to put all cell degrees of freedom first, and then all face degrees of freedom
 */

namespace HArDCore3D {


  /*!
   *	\addtogroup HybridCore
   * @{
   */
  // ----------------------------------------------------------------------------
  //                            Class definition
  // ----------------------------------------------------------------------------

  /** The HybridCore class provides convenient interfaces for performing
   * integration over mesh cells and faces and handling polynomial basis
   * functions

   **/
  /** The class also provides convenient interfaces for dealing with solutions to
      Hybrid High-Order schemes, such as the computation of integrals, norms and interpolants in the HHO space. */

  class HybridCore {

  public:
    ///@brief Class constructor: initialises the data structure with the given mesh, and desired polynomial degrees of the basis functions.
    /** The orthonormalisation comes at a cost in terms of manipulation of the basis functions. This should only be 
	use when the polynomial degree is large and/or the cell is very distorted. However, in these cases, it can 
	make a huge difference on the observed convergence rate. */
    HybridCore(
	       const Mesh* mesh_ptr, ///< A pointer to the loaded mesh 
	       const size_t K, ///< The degree of the face polynomials 
	       const size_t L, ///< The degree of the cell polynomials 
	       const std::string choice_basis = "Mon" ///< "Mon" for monomials basis, "ON" for orthonormalised basis.
	       ); 

    // Basis functions
    using cell_basis_type = std::function<double(double,double,double)>;		///< type for cell basis
    using cell_gradient_type = std::function<Eigen::Vector3d(double,double,double)>;   ///< type for gradients of cell basis
    using face_basis_type = std::function<double(double,double,double)>;   ///< type for face basis
    using tensor_function_type = std::function<Eigen::Matrix3d(double,double,double)>;   ///< type for 3D tensors basis

    /// Compute the size of the basis of 3-variate polynomials up to degree m
    size_t dim_Pcell(
		     const size_t m /**< The maximum degree of the polynomial */
		     ) const;

    /// Compute the size of the basis of 1-variate polynomials up to degree m
    size_t dim_Pface(
		     const size_t m /**< The maximum degree of the polynomial */
		     ) const;

    /// Return a reference to the i'th monomial function of the cell iT
    const cell_basis_type& cell_monomial(
					 size_t iT, /**< The global cell number of the cell */
					 size_t i   /**< The index of the desired monomial function */
					 ) const;
	
    /// Return a reference to the i'th monomial function of the face iF
    const face_basis_type& face_monomial(
					 size_t iF, /**< The global number of the face */
					 size_t i   /**< The index of the desired monomial function */
					 ) const;
	
    /// Return a reference to the i'th basis function of the cell iT
    const cell_basis_type& cell_basis(
				      size_t iT, /**< The global cell number of the cell */
				      size_t i   /**< The index of the desired basis function */
				      ) const;
	
    /// Return a reference to the i'th basis function of the face iF
    const face_basis_type& face_basis(
				      size_t iF, /**< The global face number of the face */
				      size_t i   /**< The index of the desired basis function */
				      ) const;
	
    /// Return a reference to the gradient of the i'th monomial function of the cell iT
    /** Note that the gradient functions are indexed the same as the monomial
	functions. In particular, this
	means that the first gradient function will always be identically
	zero, as it is the gradient of the constant monomial. */
    const cell_gradient_type& cell_monomials_gradient(
						      size_t iT, /**< The global cell number of the cell */
						      size_t i   /**< The index of the desired monomial function */
						      ) const;

    /// Return a reference to the gradient of the i'th basis function of the celliT
    /** Note that the gradient functions are indexed the same as the basis
	functions. In particular, this
	means that the first gradient function will always be identically
	zero, as it is the gradient of the constant basis function. */
    const cell_gradient_type& cell_gradient(
					    size_t iT, /**< The global cell number of the cell */
					    size_t i   /**< The index of the desired basis function */
					    ) const;

    Eigen::VectorXd restr(const Eigen::VectorXd &Xh, size_t iT) const;	///< Extract from a global vector Xh of unknowns the unknowns corresponding to cell iT
    double L2norm(const Eigen::VectorXd &Xh) const; ///< Compute L2 norm of a discrete function (using cell values)
    double H1norm(const Eigen::VectorXd &Xh) const; ///< Compute discrete H1 norm of a discrete function (using cell values)
    double Linf_face(const Eigen::VectorXd &Xh) const;	///< Compute maximum of the coefficients on the face basis functions

    /// Compute the interpolant in the discrete space of a continuous function
    template<typename Function>
    Eigen::VectorXd interpolate(
				const Function& f, 	///< function to interpolate
				size_t doe	///< degree of exactness of the quadrature rules to compute the interpolate
				) const;  /**< @returns The vector XTF of coefficients on the basis functions. All the cell basis functions come first (in the order of the cells), and then all the face basis functions (in the order of the faces) **/

    /// Create the matrix of L2 products of two families (f_i) and (g_j) of functions
    /// (this is not really a Gram matrix, unless the two families are the same)
    Eigen::MatrixXd gram_matrix(
				const std::vector<Eigen::ArrayXd>& f_quad, ///< Values of functions (f1,f2,...) at the quadrature nodes 
				const std::vector<Eigen::ArrayXd>& g_quad, ///< Values of functions (g1,g2,...) at the quadrature nodes 
				const size_t& nrows, ///< Number of rows of the matrix - typically number of functions f_i (but could be less) 
				const size_t& ncols, ///< Number of columns of the matrix - typically number of functions g_j (but could be less)
				const QuadratureRule& quad, 	///< Quadrature nodes for integration 
				const bool& sym,		///< True if the matrix is pseudo-symmetric (that is, #f<=#g and f_i=g_i if i<=#f) 
				std::vector<double> L2weight = {} 	///< Optional weight for the L2 product. If provided, should be a std::vector<double> of the weight at the quadrature nodes
				) const;	/**< @returns The matrix \f$(\int f_i g_j)_{i=1\ldots nrows; j=1\ldots ncols}\f$ */

    /// Overloaded version of the previous one for vector-valued functions: the functions (F_i) and (G_j) are vector-valued functions
    Eigen::MatrixXd gram_matrix(
				const std::vector<Eigen::ArrayXXd>& F_quad,		///< Values of functions (F1,F2,...) at the quadrature nodes 
				const std::vector<Eigen::ArrayXXd>& G_quad,		///< Values of functions (G1,G2,...) at the quadrature nodes 
				const size_t& nrows,		///< Number of rows of the matrix - typically number of functions F_i (but could be less) 
				const size_t& ncols,		///< Number of rows of the matrix - typically number of functions G_j (but could be less) 
				const QuadratureRule& quad,		///< Quadrature nodes for integration 
				const bool& sym,		///< True if the matrix is pseudo-symmetric (that is, #F<=#G and F_i=G_i if i<=#F) 
				std::vector<Eigen::Matrix3d> L2Weight = {}	///< Optional weight for the L2 product. If provided, should be a std::vector<Eigen::Matrix3d> of the weight at the quadrature nodes
				) const ;  /**< @returns The matrix \f$(\int F_i \cdot G_j)_{i=1\ldots nrows; j=1\ldots ncols}\f$ */
	
	
    Eigen::VectorXd compute_weights(size_t iT) const; ///< Weights to compute cell unknowns from face unknowns when l=-1
	
    /// Computes (cell or face) basis functions at the given quadrature nodes
    const std::vector<Eigen::ArrayXd> basis_quad(
						 const std::string cellface, 		///< determines the type of basis function (cell or face) we want the values of
						 const size_t iTF, 		///<	global index of the cell/face
						 const QuadratureRule quad, 	///< quadrature nodes and weights on the cell/face
             const size_t degree,    ///< the maximum polynomial degree to consider
						 const std::string type_basis = "basis"	///< optional argument to determine if we want on the monomial, or the basis functions
						 ) const;	///< @returns phi_quad[i] = array listing the nbq (=nb of quadrature nodes) values of phi_i at the quadrature nodes

    /// Compute \f$(\nabla \phi_i)_{i\in I}\f$ at the given quadrature nodes, where \f$(\phi_i)_{i\in I}\f$ are the cell basis functions
    const std::vector<Eigen::ArrayXXd> grad_basis_quad(
						       const size_t iT, 											///< global index of the cell
						       const QuadratureRule quad, 				///< quadrature rules in the cell
                   const size_t degree,                  ///< the maximum polynomial degree to consider
						       const std::string type_basis = "basis"	///< optional argument to determine if we want on the monomial, or the basis functions
						       ) const; ///< @returns dphi_quad[i]: array of size 3*nbq (where nbq=nb of quadrature nodes), with each column being \f$\nabla \phi_i\f$ at the corresponding quadrature node 

    /// Evaluates a discrete function in the cell iT at point (x,y,z)
    double evaluate_in_cell(const Eigen::VectorXd XTF, size_t iT, double x, double y, double z) const; 
    /// Evaluates a discrete function on the face iF at point (x,y,z)
    double evaluate_in_face(const Eigen::VectorXd XTF, size_t iF, double x, double y, double z) const;


    inline const Mesh* get_mesh_ptr() const;    ///< returns a pointer to the mesh
    inline size_t K() const;    ///< polynomial degree of edge unknowns
    inline int L() const;    ///< polynomial degree of cell unknowns
    inline int Ldeg() const;    ///< usually equal to L, but put at 0 if L=-1
    inline size_t ntotal_dofs() const;	///< Total number of degrees of freedom
    inline size_t nlocal_cell_dofs() const; ///< number of degrees of freedom in each cell (dimension of polynomial space)
    inline size_t ntotal_cell_dofs() const; ///< total number of cell degrees of freedom
    inline size_t nlocal_face_dofs() const;	///< number of degrees of freedom on each face (dimension of polynomial space)
    inline size_t ntotal_face_dofs() const;	///< total number of face degrees of freedom
    inline size_t ninternal_face_dofs() const;	///< total number of face degrees of freedom for internal faces
    inline size_t nboundary_face_dofs() const;	///< total number of face degrees of freedom for boundary faces
    inline size_t nhighorder_dofs() const;	///< total number of cell degrees of freedom with polynomials up to order k+1
    inline size_t ngradient_dofs() const;  ///< total number of degrees of freedom for gradients

    /// To integrate a function over a cell
    template <typename Function>
    void quadrature_over_cell(const size_t iT, const Function& f) const;
    /// To integrate a function over a face
    template <typename Function>
    void quadrature_over_face(const size_t iF, const Function& f) const;
    ///Integrates a function over a cell. Use with parcimony, expensive (re-compute quadratures)
    template <typename Function>
    double integrate_over_cell(const size_t iT, const Function& f) const;
    ///Integrates a function over a face. Use with parcimony, expensive (re-compute quadratures)
    template <typename Function>
    double integrate_over_face(const size_t iF, const Function& f) const;
    ///Integrates a function over the domaine. Use with parcimony, expensive (re-compute quadratures)
    template <typename Function>
    double integrate_over_domain(const Function& f) const;

    /// From a hybrid function, computes a vector of values at the vertices of the mesh
    Eigen::VectorXd VertexValues(
				 const Eigen::VectorXd Xh,   ///< hybrid function (cell and face polynomials)
				 const std::string from_dofs ///< Type of unknowns to use: "cell" or "face"
				 );

  private:
    std::pair<std::vector<HybridCore::cell_basis_type>,
	      std::vector<HybridCore::cell_gradient_type> >
    create_cell_monomials(const size_t iT) const; ///< creates the monomial functions for cell iT of degree K.
    std::vector<HybridCore::face_basis_type> create_face_monomials(const size_t iF) const;	///< creates the monomial functions for face iF of degree L
    std::tuple<std::vector<cell_basis_type>, std::vector<cell_gradient_type>, Eigen::MatrixXd>
    create_basis(const std::string cellface, const size_t iTF);	///< creates the basis functions, and the matrix of passage from monomial to basis functions, depending on 'choice_basis'

    // Mesh data
    const Mesh* _mesh_ptr;  // Shared ptr to mesh data
    // const Mesh& _mesh;  // Const-reference to the mesh data for
    //                      // immutable access
    /// Degree of the face polynomials
    const size_t _K;
    /// Degree of the cell polynomials (can be -1).
    const int _L;
    const size_t _Ldeg;
    /// Number of degrees of freedom per cell polynomial
    const size_t _nlocal_cell_dofs;
    /// Number of degrees of freedom per cell for high-order (K+1) polynomials
    //const int _nhighorder_dofs;
    /** Number of degrees of freedom in the space of gradients of high-order
	cell polynomials*/

    const size_t _nlocal_face_dofs;
    /// Number of degrees of freedom per cell for high-order (K+1) polynomials
    const size_t _nhighorder_dofs;
    /// Number of degrees of freedom in the space of gradients of high-order cell polynomials
    const size_t _ngradient_dofs;
    /// Total number of cell degrees of freedom over the entire mesh
    const size_t _ntotal_cell_dofs;
    /// Total number of face degrees of freedom over the entire mesh
    const size_t _ntotal_face_dofs;
    /// Total number of face degrees of freedom inside the domain
    const size_t _ninternal_face_dofs;
    /// Total number of face degrees of freedom on the boundary of the domain
    const size_t _nboundary_face_dofs;

    /// Total number of degrees of freedom over the entire mesh
    const size_t _ntotal_dofs;

    // Collections of monomial and basis functions for each cell and face
    std::string _choice_basis;
    std::vector<std::vector<cell_basis_type> > _cell_monomials;
    std::vector<std::vector<cell_gradient_type> > _cell_monomials_gradients;
    std::vector<std::vector<cell_basis_type> > _cell_bases;
    std::vector<std::vector<cell_gradient_type> > _cell_gradients;
    std::vector<std::vector<face_basis_type> > _face_monomials;
    std::vector<std::vector<face_basis_type> > _face_bases;
    std::vector<Eigen::MatrixXd> _M_cell_basis;
    std::vector<Eigen::MatrixXd> _M_face_basis;

    // offset for quadrature rules, should be 0 except for testing purposes
    int _offset_doe;	

  };



  // ----------------------------------------------------------------------------
  //                            Implementation
  // ----------------------------------------------------------------------------

  // ----------------------------------------------------------------------------
  // ------- 'Easy' integration routines, expensive (re-compute quad rules), use as little as possible


  template <typename Function>
  void HybridCore::quadrature_over_cell(const size_t iT, const Function& f) const {
    assert(iT < _mesh_ptr->n_cells());
    QuadratureRule quadT = generate_quadrature_rule(*_mesh_ptr->cell(iT), 2 * _Ldeg + 2);
    for (size_t iqn = 0; iqn < quadT.size(); iqn++) {
      f(iqn, quadT[iqn].x, quadT[iqn].y, quadT[iqn].z, quadT[iqn].w);
    }
  }

  template <typename Function>
  void HybridCore::quadrature_over_face(const size_t iF, const Function& f) const {
    assert(iF < _mesh_ptr->n_faces());

    QuadratureRule quadF = generate_quadrature_rule(*_mesh_ptr->face(iF), 2 * _K + 2);
    for (size_t iqn = 0; iqn < quadF.size(); iqn++) {
      f(iqn, quadF[iqn].x, quadF[iqn].y, quadF[iqn].z, quadF[iqn].w);
    }
  }

  template <typename Function>
  double HybridCore::integrate_over_cell(const size_t iT, const Function& f) const {
    assert(iT < _mesh_ptr->n_cells());
    double ans = 0.0;
    quadrature_over_cell(iT, [&ans, &f](size_t iQN, double x, double y, double z,
					double w) { ans += w * f(x, y, z); });
    return ans;
  }

  template <typename Function>
  double HybridCore::integrate_over_face(const size_t iF, const Function& f) const {
    assert(iF < _mesh_ptr->n_faces());
    double ans = 0.0;
    quadrature_over_face(iF, [&ans, &f](size_t iQN, double x, double y, double z,
					double w) { ans += w * f(x, y, z); });
    return ans;
  }

  template <typename Function>
  double HybridCore::integrate_over_domain(const Function& f) const{
    double value = 0.0;
    for (size_t iT = 0; iT < _mesh_ptr->n_cells(); iT++){
      value += integrate_over_cell(iT, f);
    }
    return value;
  }





  // ----------------------------------------------------------
  // ------- Interpolates continuous function on discrete space

  template<typename Function>
  Eigen::VectorXd HybridCore::interpolate(const Function& f, size_t doe) const {
    Eigen::VectorXd XTF = Eigen::VectorXd::Zero(_ntotal_dofs);

    // Face projections
    for (size_t iF = 0; iF < _mesh_ptr->n_faces(); iF++) {
      Face* face = _mesh_ptr->face(iF);

      // Mass matrix on face
      QuadratureRule quadF = generate_quadrature_rule(*face, doe);
      std::vector<Eigen::ArrayXd> phiF_quadF = basis_quad("face", iF, quadF, _K);
      Eigen::MatrixXd MF = gram_matrix(phiF_quadF, phiF_quadF, _nlocal_face_dofs, _nlocal_face_dofs, quadF, true);

      // Vector of integral of f against face basis functions
      Eigen::VectorXd bF = Eigen::VectorXd::Zero(_nlocal_face_dofs);
      for (size_t i = 0; i < _nlocal_face_dofs; i++) {
        for (size_t iqn = 0; iqn < quadF.size(); iqn++){
          bF(i) += quadF[iqn].w * phiF_quadF[i](iqn) * f(quadF[iqn].x, quadF[iqn].y, quadF[iqn].z);
        }
      }

      // Vector of coefficients (on cell basis functions) of the L2(F) projection of f
      Eigen::VectorXd UF = MF.ldlt().solve(bF);

      // Fill in he complete vector of DOFs
      XTF.segment(_ntotal_cell_dofs + iF * _nlocal_face_dofs, _nlocal_face_dofs) = UF;
    }

    for (size_t iT = 0; iT < _mesh_ptr->n_cells(); iT++) {
      Cell* cell = _mesh_ptr->cell(iT);
      size_t nfaces = cell->n_faces();

      // Mass matrix in cell
      QuadratureRule quadT = generate_quadrature_rule(*cell, doe);
      std::vector<Eigen::ArrayXd> phiT_quadT = basis_quad("cell", iT, quadT, _Ldeg);
      Eigen::MatrixXd MT = gram_matrix(phiT_quadT, phiT_quadT, _nlocal_cell_dofs, _nlocal_cell_dofs, quadT, true);

      // Vector of integrals of f against basis functions
      Eigen::VectorXd bT = Eigen::VectorXd::Zero(_nlocal_cell_dofs);
      for (size_t i = 0; i < _nlocal_cell_dofs; i++) {
      	for (size_t iqn = 0; iqn < quadT.size(); iqn++){
	        bT(i) += quadT[iqn].w * phiT_quadT[i](iqn) * f(quadT[iqn].x, quadT[iqn].y, quadT[iqn].z);
	      }
      }

      // Vector of coefficients (on cell basis functions) of the L2(T) projection of f
      Eigen::VectorXd UT = MT.ldlt().solve(bT);

      // Fill in the complete vector of DOFs
      size_t offset_T = iT * _nlocal_cell_dofs;
      XTF.segment(offset_T, _nlocal_cell_dofs) = UT;

      // Special case of L=-1, we replace the previously computed cell value with the proper average of face values
      if ( _L==-1 ){
	// Weights corresponding to the values in cells/on faces
	Eigen::VectorXd barycoefT = compute_weights(iT);
	// We transform these weights so that they apply to the coefficients on the basis functions
	Eigen::Vector3d xT = cell->center_mass();
	double phiT_cst = cell_basis(iT,0)(xT.x(), xT.y(), xT.z());
	for (size_t ilF = 0; ilF < nfaces; ilF++){
	  Eigen::Vector3d xF = cell->face(ilF)->center_mass();
	  size_t iF = cell->face(ilF)->global_index();
	  double phiF_cst = face_basis(iF,0)(xF.x(), xF.y(), xF.z());
	  barycoefT(ilF) *= phiF_cst / phiT_cst;
	}

	XTF(iT) = 0;
	for (size_t ilF = 0; ilF < nfaces; ilF++) {
	  size_t iF = cell->face(ilF)->global_index();
	  XTF(iT) += barycoefT(ilF) * XTF(_ntotal_cell_dofs + iF);
	}
      }

    }

    return XTF;
  }



  // --------------------------------------------------------------------------------------------------
  // ------- Functions that return class elements

  size_t HybridCore::nlocal_cell_dofs() const { return _nlocal_cell_dofs; }
  size_t HybridCore::ntotal_cell_dofs() const { return _ntotal_cell_dofs; }
  size_t HybridCore::nlocal_face_dofs() const { return _nlocal_face_dofs; }
  size_t HybridCore::ntotal_face_dofs() const { return _ntotal_face_dofs; }
  size_t HybridCore::nhighorder_dofs() const { return _nhighorder_dofs; }
  size_t HybridCore::ngradient_dofs() const { return _ngradient_dofs; }

  size_t HybridCore::ninternal_face_dofs() const { return _ninternal_face_dofs; }
  size_t HybridCore::nboundary_face_dofs() const { return _nboundary_face_dofs; }
  size_t HybridCore::ntotal_dofs() const { return _ntotal_dofs; }

  const Mesh* HybridCore::get_mesh_ptr() const {return _mesh_ptr;}
  size_t HybridCore::K() const {return _K;}
  int HybridCore::L() const {return _L;}
  int HybridCore::Ldeg() const {return _Ldeg;}

  //@}

}  // end of namespace HArDCore3D

#endif /* HYBRIDCORE_HPP */
