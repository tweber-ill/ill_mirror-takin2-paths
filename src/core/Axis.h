/**
 * instrument axis
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
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

#ifndef __INSTR_AXIS_H__
#define __INSTR_AXIS_H__

#include <optional>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/signals2/signal.hpp>

#include "types.h"
#include "Geometry.h"


// forward declaration
class Instrument;



// ----------------------------------------------------------------------------
// instrument axis
// ----------------------------------------------------------------------------
enum class AxisAngle
{
	INCOMING,	// defined with respect to incoming axis
	INTERNAL,	// defined with respect to local rotation
	OUTGOING	// defined with respect to outgoing axis
};


class Axis
{
public:
	// constructor and destructor
	Axis(const std::string &id = "", const Axis *prev = nullptr,
		const Axis *next = nullptr, Instrument *instr = nullptr);
	~Axis();

	// copy constructor and operator
	Axis(const Axis& axis);
	const Axis& operator=(const Axis& axis);

	void SetPreviousAxis(const Axis* axis);
	void SetNextAxis(const Axis* axis);
	void SetParentInstrument(Instrument* instr);

	void Clear();
	bool Load(const boost::property_tree::ptree& prop);
	boost::property_tree::ptree Save() const;

	const std::string& GetId() const { return m_id; }
	const t_vec& GetZeroPos() const { return m_pos; }

	t_real GetAxisAngleIn() const { return m_angle_in; }
	t_real GetAxisAngleOut() const { return m_angle_out; }
	t_real GetAxisAngleInternal() const { return m_angle_internal; }

	void SetAxisAngleIn(t_real angle);
	void SetAxisAngleOut(t_real angle);
	void SetAxisAngleInternal(t_real angle);

	t_real GetAxisAngleInLowerLimit() const;
	t_real GetAxisAngleInUpperLimit() const;
	t_real GetAxisAngleOutLowerLimit() const;
	t_real GetAxisAngleOutUpperLimit() const;
	t_real GetAxisAngleInternalLowerLimit() const;
	t_real GetAxisAngleInternalUpperLimit() const;

	void SetAxisAngleInLowerLimit(t_real angle);
	void SetAxisAngleInUpperLimit(t_real angle);
	void SetAxisAngleOutLowerLimit(t_real angle);
	void SetAxisAngleOutUpperLimit(t_real angle);
	void SetAxisAngleInternalLowerLimit(t_real angle);
	void SetAxisAngleInternalUpperLimit(t_real angle);

	t_real GetAxisAngleInSpeed() const;
	t_real GetAxisAngleInternalSpeed() const;
	t_real GetAxisAngleOutSpeed() const;

	void SetAxisAngleInSpeed(t_real speed);
	void SetAxisAngleInternalSpeed(t_real speed);
	void SetAxisAngleOutSpeed(t_real speed);

	// which==1: in, which==2: internal, which==3: out
	const t_mat& GetTrafo(AxisAngle which=AxisAngle::INCOMING) const;
	void UpdateTrafos() const;
	void TrafosNeedUpdate() const;

	const std::vector<std::shared_ptr<Geometry>>&
		GetComps(AxisAngle which = AxisAngle::INCOMING) const;

	bool IsObjectOnAxis(const std::string& obj, AxisAngle ax) const;

	std::vector<ObjectProperty> GetProperties() const;
	void SetProperties(const std::vector<ObjectProperty>& props);

	std::vector<ObjectProperty> GetProperties(const std::string& objname) const;
	std::tuple<bool, std::shared_ptr<Geometry>> SetProperties(
		const std::string& objname, const std::vector<ObjectProperty>& props);

private:
	// identifier
	std::string m_id{};
	// previous and next axis
	const Axis *m_prev = nullptr, *m_next = nullptr;
	// parent instrument
	Instrument *m_instr = nullptr;

	// trafo matrices
	mutable t_mat m_trafoIncoming = tl2::unit<t_mat>(4);
	mutable t_mat m_trafoInternal = tl2::unit<t_mat>(4);
	mutable t_mat m_trafoOutgoing = tl2::unit<t_mat>(4);
	mutable bool m_trafos_need_update = true;

	// coordinate origin
	t_vec m_pos = tl2::create<t_vec>({0,0});

	// angle of incoming axis and outgoing axis
	t_real m_angle_in = 0, m_angle_out = 0;
	// internal rotation angle
	t_real m_angle_internal = 0;

	// optional angular limits
	std::optional<t_real> m_angle_in_limits[2];
	std::optional<t_real> m_angle_internal_limits[2];
	std::optional<t_real> m_angle_out_limits[2];

	// optional angular speeds
	std::optional<t_real> m_angle_in_speed = std::nullopt;
	std::optional<t_real> m_angle_internal_speed = std::nullopt;
	std::optional<t_real> m_angle_out_speed = std::nullopt;

	// components relative to incoming and outgoing axis
	std::vector<std::shared_ptr<Geometry>> m_comps_in = {};
	std::vector<std::shared_ptr<Geometry>> m_comps_out = {};
	// components rotated internally
	std::vector<std::shared_ptr<Geometry>> m_comps_internal{};
};
// ----------------------------------------------------------------------------


#endif
