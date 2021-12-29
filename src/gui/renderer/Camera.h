/**
 * camera
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @note initially forked from my tlibs2 library: https://code.ill.fr/scientific-software/takin/tlibs2/-/blob/master/libs/qt/glplot.h
 * @license GPLv3, see 'LICENSE' file
 *
 * References:
 *   - http://doc.qt.io/qt-5/qopenglwidget.html#details
 *   - http://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl/threadedqopenglwidget
 *   - http://doc.qt.io/qt-5/qtgui-openglwindow-example.html
 *   - http://doc.qt.io/qt-5/qopengltexture.html
 *   - (Sellers 2014) G. Sellers et al., ISBN: 978-0-321-90294-8 (2014).
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite)
 * Copyright (C) 2021  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                     Grenoble, France).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 */

#ifndef __PATHS_RENDERER_CAM_H__
#define __PATHS_RENDERER_CAM_H__

#include <tuple>

#include "tlibs2/libs/maths.h"


template<class t_mat, class t_vec, class t_real>
requires tl2::is_mat<t_mat> && tl2::is_vec<t_vec>
class Camera
{
public:
	Camera() = default;
	~Camera() = default;


	/**
	 * update camera matrices
	 */
	void Update()
	{
		t_mat matCamTrans = m_matTrans;
		matCamTrans(2,3) = 0.;
		t_mat matCamTrans_inv = matCamTrans;
		matCamTrans_inv(0,3) = -matCamTrans(0,3);
		matCamTrans_inv(1,3) = -matCamTrans(1,3);
		matCamTrans_inv(2,3) = -matCamTrans(2,3);

		const t_vec vecCamDir[2] =
		{
			tl2::create<t_vec>({1., 0., 0.}),
			tl2::create<t_vec>({0. ,0., 1.})
		};

		m_matRot = tl2::hom_rotation<t_mat, t_vec>(
			vecCamDir[0], m_theta, 0);
		m_matRot *= tl2::hom_rotation<t_mat, t_vec>(
			vecCamDir[1], m_phi, 0);

		m_mat = tl2::unit<t_mat>();
		m_mat *= m_matTrans;
		m_mat(2,3) /= m_zoom;
		m_mat *= matCamTrans_inv * m_matRot * matCamTrans;
		std::tie(m_mat_inv, std::ignore) = tl2::inv<t_mat>(m_mat);
	}


	/**
	 * centre camera on object matrix
	 */
	void Centre(const t_mat& objmat)
	{
		m_matTrans(0,3) = -objmat(0,3);
		m_matTrans(1,3) = -objmat(1,3);
		//m_matTrans(2,3) = -objmat(2,3);
	}


	/**
	 * set the camera's field of view
	 */
	void SetFOV(t_real angle)
	{
		m_FOV = angle;
	}


	/**
	 * get the camera's field of view
	 */
	t_real GetFOV() const
	{
		return m_FOV;
	}


	/**
	 * set the camera zoom
	 */
	void SetZoom(t_real zoom)
	{
		m_zoom = zoom;
	}


	/**
	 * get the camera's zoom factor
	 */
	t_real GetZoom() const
	{
		return m_zoom;
	}


	/**
	 * set the camera position
	 */
	void SetPosition(const t_vec& pos)
	{
		m_matTrans(0, 3) = pos[0];
		m_matTrans(1, 3) = pos[1];
		m_matTrans(2, 3) = pos[2];
	}


	/**
	 * get the camera position
	 */
	t_vec GetPosition() const
	{
		return tl2::create<t_vec>(
		{
			m_matTrans(0, 3),
			m_matTrans(1, 3),
			m_matTrans(2, 3),
		});
	}


	/**
	 * set the rotation angles
	 */
	void SetRotation(const t_real phi, t_real theta)
	{
		m_phi_saved = m_phi = phi;
		m_theta_saved = m_theta = theta;
	}


	/**
	 * get the camera's rotation matrix
	 */
	std::tuple<t_real, t_real> GetRotation() const
	{
		return std::make_tuple(m_phi, m_theta);
	}


	/**
	 * save the current rotation angles
	 */
	void SaveRotation()
	{
		m_phi_saved = m_phi;
		m_theta_saved = m_theta;
	}


	/**
	 * rotate the camera by the given delta angles
	 */
	void Rotate(t_real dphi, t_real dtheta)
	{
		m_phi = dphi + m_phi_saved;
		t_real theta_new = dtheta + m_theta_saved;

		// wrap around phi angle
		m_phi = tl2::mod_pos<t_real>(
			m_phi, t_real(2)*tl2::pi<t_real>);

		// restrict theta angle
		m_theta = tl2::clamp<t_real>(
			theta_new, -tl2::pi<t_real>*t_real(0.5), 0);
	}


	/**
	 * translate the camera by the given deltas
	 */
	void Translate(t_real dx, t_real dy, t_real dz)
	{
		t_vec xdir = tl2::row<t_mat, t_vec>(m_matRot, 0);
		t_vec ydir = tl2::row<t_mat, t_vec>(m_matRot, 1);
		t_vec zdir = tl2::row<t_mat, t_vec>(m_matRot, 2);

		t_vec xinc = xdir * dx;
		t_vec yinc = ydir * dy;
		t_vec zinc = zdir * dz;

		m_matTrans(0,3) += xinc[0] + yinc[0] + zinc[0];
		m_matTrans(1,3) += xinc[1] + yinc[1] + zinc[1];
		m_matTrans(2,3) += xinc[2] + yinc[2] + zinc[2];
	}


	/**
	 * zoom
	 */
	void Zoom(t_real zoom)
	{
		m_zoom *= std::pow(t_real(2), zoom);
	}


	/**
	 * get the camera's full transformation matrix
	 */
	const t_mat& GetMatrix() const
	{
		return m_mat;
	}


	/**
	 * get the camera's inverse transformation matrix
	 */
	const t_mat& GetInverseMatrix() const
	{
		return m_mat_inv;
	}


private:
	// full transformation matrix and inverse
	t_mat m_mat = tl2::unit<t_mat>();
	t_mat m_mat_inv = tl2::unit<t_mat>();

	// rotation and translation matrices
	t_mat m_matRot = tl2::unit<t_mat>();
	t_mat m_matTrans = tl2::create<t_mat>
		({ 1,0,0,0, 0,1,0,0, 0,0,1,-15, 0,0,0,1 });

	// field of view
	t_real m_FOV = tl2::pi<t_real>*t_real(0.5);

	// camera rotation
	t_real m_phi = 0, m_theta = 0;
	t_real m_phi_saved = 0, m_theta_saved = 0;

	// camera zoom
	t_real m_zoom = 1.;
};


#endif
