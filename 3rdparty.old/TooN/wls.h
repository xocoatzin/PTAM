// -*- c++ -*-

// Copyright (C) 2005,2009 Tom Drummond (twd20@cam.ac.uk)
//
// This file is part of the TooN Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

#ifndef TOON_INCLUDE_WLS_H
#define TOON_INCLUDE_WLS_H

#include <TooN/TooN.h>
#include <TooN/Cholesky.h>
#include <TooN/helpers.h>

#include <cmath>

namespace TooN {

/// Performs weighted least squares computation.
/// @param Size The number of dimensions in the system
/// @param Precision The numerical precision used (double, float etc)
/// @param Decomposition The class used to invert the inverse Covariance matrix (must have one integer size and one typename precision template arguments) this is Cholesky by default, but could also be SQSVD
/// @ingroup gEquations
template <int Size=Dynamic, class Precision=double,
		  template<int Size, class Precision> class Decomposition = Cholesky>
class WLS {
public:

	/// Default constructor or construct with the number of dimensions for the Dynamic case
	WLS(int size=0) :
		my_C_inv(size,size),
		my_vector(size),
		my_decomposition(size),
		my_mu(size)
	{
		clear();
	}

	/// Clear all the measurements and apply a constant regularisation term. 
	/// Equates to a prior that says all the parameters are zero with \f$\sigma^2 = \frac{1}{\text{val}}\f$.
	/// @param prior The strength of the prior
	void clear(){
		my_C_inv = Zeros;
		my_vector = Zeros;
	}

	/// Applies a constant regularisation term. 
	/// Equates to a prior that says all the parameters are zero with \f$\sigma^2 = \frac{1}{\text{val}}\f$.
	/// @param val The strength of the prior
	void add_prior(Precision val){
		for(int i=0; i<my_C_inv.num_rows(); i++){
			my_C_inv(i,i)+=val;
		}
	}
  
	/// Applies a regularisation term with a different strength for each parameter value. 
	/// Equates to a prior that says all the parameters are zero with \f$\sigma_i^2 = \frac{1}{\text{v}_i}\f$.
	/// @param v The vector of priors
	template<class B2>
	void add_prior(const Vector<Size,Precision,B2>& v){
		SizeMismatch<Size,Size>::test(my_C_inv.num_rows(), v.size());
		for(int i=0; i<my_C_inv.num_rows(); i++){
			my_C_inv(i,i)+=v[i];
		}
	}

	/// Applies a whole-matrix regularisation term. 
	/// This is the same as adding the \f$m\f$ to the inverse covariance matrix.
	/// @param m The inverse covariance matrix to add
	template<class B2>
	void add_prior(const Matrix<Size,Size,Precision,B2>& m){
		my_C_inv+=m;
	}

	/// Add a single measurement 
	/// @param m The value of the measurement
	/// @param J The Jacobian for the measurement \f$\frac{\partial\text{m}}{\partial\text{param}_i}\f$
	/// @param weight The inverse variance of the measurement (default = 1)
	template<class B2>
	inline void add_mJ(Precision m, const Vector<Size, Precision, B2>& J, Precision weight = 1) {
		Vector<Size,Precision> Jw = J*weight;
		my_C_inv += Jw.as_col() * J.as_row();
		my_vector+= m*Jw;
	}

	/// Add multiple measurements at once (much more efficiently)
	/// @param N The number of measurements
	/// @param m The measurements to add
	/// @param J The Jacobian matrix \f$\frac{\partial\text{m}_i}{\partial\text{param}_j}\f$
	/// @param invcov The inverse covariance of the measurement values
	template<int N, class B1, class B2, class B3>
	inline void add_mJ(const Vector<N,Precision,B1>& m,
					   const Matrix<Size,N,Precision,B2>& J,
					   const Matrix<N,N,Precision,B3>& invcov){
		Matrix<Size,N,Precision> temp =  J * invcov;
		my_C_inv += temp * J.T();
		my_vector += temp * m;
	}


	/// Process all the measurements and compute the weighted least squares set of parameter values
	/// stores the result internally which can then be accessed by calling get_mu()
	void compute(){
		my_decomposition.compute(my_C_inv);
		my_mu=my_decomposition.backsub(my_vector);
	}

	/// Combine measurements from two WLS systems
	/// @param meas The measurements to combine with
	void operator += (const WLS& meas){
		my_vector+=meas.my_vector;
		my_C_inv += meas.my_C_inv;
	}

	/// Returns the inverse covariance matrix
	Matrix<Size,Size,Precision>& get_C_inv() {return my_C_inv;}
	/// Returns the inverse covariance matrix
	const Matrix<Size,Size,Precision>& get_C_inv() const {return my_C_inv;}
	Vector<Size,Precision>& get_mu(){return my_mu;}
	const Vector<Size,Precision>& get_mu() const {return my_mu;}
	Vector<Size,Precision>& get_vector(){return my_vector;}
	const Vector<Size,Precision>& get_vector() const {return my_vector;}
	Decomposition<Size,Precision>& get_decomposition(){return my_decomposition;}
	const Decomposition<Size,Precision>& get_decomposition() const {return my_decomposition;}


private:
	Matrix<Size,Size,Precision> my_C_inv;
	Vector<Size,Precision> my_vector;
	Decomposition<Size,Precision> my_decomposition;
	Vector<Size,Precision> my_mu;

	// comment out to allow bitwise copying
	WLS( WLS& copyof );
	int operator = ( WLS& copyof );
};

}

#endif
