/**
 * triple axis angle calculation
 * @author Tobias Weber <tweber@ill.fr>
 * @date jul-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "TasCalculator.h"


void TasCalculator::SetMonochromatorD(t_real d)
{
	m_dspacings[0] = d;
}


void TasCalculator::SetAnalyserD(t_real d)
{
	m_dspacings[1] = d;
}


void TasCalculator::SetSampleAngleOffset(t_real offs)
{
	m_a3Offs = offs;
}


void TasCalculator::SetScatteringSenses(bool monoccw, bool sampleccw, bool anaccw)
{
	m_sensesCCW[0] = (monoccw ? 1 : -1);
	m_sensesCCW[1] = (sampleccw ? 1 : -1);
	m_sensesCCW[2] = (anaccw ? 1 : -1);
}


const t_real* TasCalculator::GetScatteringSenses() const
{
	return m_sensesCCW;
}


void TasCalculator::SetSampleLatticeConstants(t_real a, t_real b, t_real c)
{
	m_lattice = tl2::create<t_vec>({ a, b, c });
}


void TasCalculator::SetSampleLatticeAngles(t_real alpha, t_real beta, t_real gamma, bool deg)
{
	if(deg)
	{
		alpha = alpha / t_real(180) * tl2::pi<t_real>;
		beta = beta / t_real(180) * tl2::pi<t_real>;
		gamma = gamma / t_real(180) * tl2::pi<t_real>;
	}

	m_angles = tl2::create<t_vec>({ alpha, beta, gamma });
}


void TasCalculator::SetSampleScatteringPlane(t_real vec1_x, t_real vec1_y, t_real vec1_z,
	t_real vec2_x, t_real vec2_y, t_real vec2_z)
{
	m_plane_rlu[0] = tl2::create<t_vec>({vec1_x, vec1_y, vec1_z});
	m_plane_rlu[1] = tl2::create<t_vec>({vec2_x, vec2_y, vec2_z});

	UpdateScatteringPlane();
}


const t_mat& TasCalculator::GetB() const
{
	return m_B;
}


const t_mat& TasCalculator::GetUB() const
{
	return m_UB;
}


void TasCalculator::UpdateB()
{
	m_B = tl2::B_matrix<t_mat>(
		m_lattice[0], m_lattice[1], m_lattice[2],
		m_angles[0], m_angles[1], m_angles[2]);

	UpdateScatteringPlane();
}


void TasCalculator::UpdateScatteringPlane()
{
	m_plane_rlu[2] = tl2::cross<t_mat, t_vec>(
		m_B, m_plane_rlu[0], m_plane_rlu[1]);
}


void TasCalculator::UpdateUB()
{
	m_UB = tl2::UB_matrix<t_mat, t_vec>(m_B,
		m_plane_rlu[0], m_plane_rlu[1], m_plane_rlu[2]);

	/*using namespace tl2_ops;
	std::cout << "vec1: " << m_plane_rlu[0] << std::endl;
	std::cout << "vec2: " << m_plane_rlu[1] << std::endl;
	std::cout << "vec3: " << m_plane_rlu[2] << std::endl;
	std::cout << "B matrix: " << m_B << std::endl;
	std::cout << "UB matrix: " << m_UB << std::endl;*/
}


/**
 * calculate instrument coordinates in xtal system
 */
std::tuple<std::optional<t_vec>, t_real>
TasCalculator::GetQE(t_real monoXtalAngle, t_real anaXtalAngle,
	t_real sampleXtalAngle, t_real sampleScAngle) const
{
	// get crystal coordinates corresponding to current instrument position
	t_real ki = tl2::calc_tas_k<t_real>(monoXtalAngle, m_dspacings[0]);
	t_real kf = tl2::calc_tas_k<t_real>(anaXtalAngle, m_dspacings[1]);
	t_real Q = tl2::calc_tas_Q_len<t_real>(ki, kf, sampleScAngle);
	t_real E = tl2::calc_tas_E<t_real>(ki, kf);

	std::optional<t_vec> Qrlu = tl2::calc_tas_hkl<t_mat, t_vec, t_real>(
		m_B, ki, kf, Q, sampleXtalAngle,
		m_plane_rlu[0], m_plane_rlu[2],
		m_sensesCCW[1], m_a3Offs);

	return std::make_tuple(Qrlu, E);
}


/**
 * calculate instrument angles
 */
TasAngles TasCalculator::GetAngles(
	t_real h, t_real k, t_real l, t_real ki, t_real kf) const
{
	TasAngles angles;

	std::optional<t_real> a1 = tl2::calc_tas_a1<t_real>(ki, m_dspacings[0]);
	std::optional<t_real> a5 = tl2::calc_tas_a1<t_real>(kf, m_dspacings[1]);

	if(a1)
	{
		*a1 *= m_sensesCCW[0];
		angles.mono_ok = true;
		angles.monoXtalAngle = *a1;
	}

	if(a5)
	{
		*a5 *= m_sensesCCW[2];
		angles.ana_ok = true;
		angles.anaXtalAngle = *a5;
	}

	t_vec Q = tl2::create<t_vec>({ h, k, l });
	std::tie(angles.sample_ok,
		angles.sampleXtalAngle, 
		angles.sampleScatteringAngle,
		angles.distance) = calc_tas_a3a4<t_mat, t_vec, t_real>(
		m_B, ki, kf, Q,
		m_plane_rlu[0], m_plane_rlu[2],
		m_sensesCCW[1], m_a3Offs);

	return angles;
}
