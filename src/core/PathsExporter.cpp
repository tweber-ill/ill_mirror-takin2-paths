/**
 * export paths to instrument control systems
 * @author Tobias Weber <tweber@ill.fr>
 * @date jul-2021
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

#include "PathsBuilder.h"
#include "PathsExporter.h"

#include <fstream>


/**
 * export the path as raw data
 */
bool PathsExporterRaw::Export(const PathsBuilder* builder,
	const std::vector<t_vec2>& path, bool path_in_rad) const
{
	if(!builder)
		return false;

	std::ofstream ofstr(m_filename);
	if(!ofstr)
		return false;

	ofstr.precision(m_prec);

	// output tas properties
	const TasCalculator* tascalc = builder->GetTasCalculator();
	if(tascalc)
	{
		auto kfix = tascalc->GetKfix();

		ofstr << "#\n";
		ofstr << "# k_fix = " << std::get<0>(kfix) << "\n";
		ofstr << "# k_fix_is_kf = " << std::boolalpha << std::get<1>(kfix) << "\n";
		ofstr << "#\n";
	}

	// output path vertices
	ofstr << "# "
		<< std::right << std::setw(m_prec*2-2) << "a4 (deg)" << " "
		<< std::right << std::setw(m_prec*2) << "a2 (deg)" << "\n";

	for(const auto& vec : path)
	{
		t_real a4 = vec[0];
		t_real a2 = vec[1];

		if(path_in_rad)
		{
			a4 = a4 / tl2::pi<t_real> * t_real(180);
			a2 = a2 / tl2::pi<t_real> * t_real(180);
		}

		ofstr
			<< std::right << std::setw(m_prec*2) << a4 << " "
			<< std::right << std::setw(m_prec*2) << a2 << "\n";
	}

	ofstr.flush();
    return true;
}


/**
 * export the path into Nomad commands
 */
bool PathsExporterNomad::Export(const PathsBuilder* builder,
	const std::vector<t_vec2>& path, bool path_in_rad) const
{
	if(!builder)
		return false;

	std::ofstream ofstr(m_filename);
	if(!ofstr)
		return false;

	ofstr.precision(m_prec);

	// set-up tas properties
	const TasCalculator* tascalc = builder->GetTasCalculator();
	if(tascalc)
	{
		auto kfix = tascalc->GetKfix();

		if(std::get<1>(kfix))
			ofstr << "dr kf " << std::get<0>(kfix) << "\n";
		else
			ofstr << "dr ki " << std::get<0>(kfix) << "\n";

		ofstr << "\n";
	}

	// output motor drive commands
	for(const auto& vec : path)
	{
		t_real a4 = vec[0];
		t_real a2 = vec[1];

		if(path_in_rad)
		{
			a4 = a4 / tl2::pi<t_real> * t_real(180);
			a2 = a2 / tl2::pi<t_real> * t_real(180);
		}

		ofstr
			<< "dr a4 " << std::left << std::setw(m_prec*2) << a4 << " "
			<< "a2 " << std::left << std::setw(m_prec*2) << a2 << "\n";
	}

	ofstr.flush();
    return true;
}


/**
 * export the path into Nicos commands
 */
bool PathsExporterNicos::Export(const PathsBuilder* builder,
	const std::vector<t_vec2>& path, bool path_in_rad) const
{
	if(!builder)
		return false;

	std::ofstream ofstr(m_filename);
	if(!ofstr)
		return false;

	ofstr.precision(m_prec);

	// set-up tas properties
	const TasCalculator* tascalc = builder->GetTasCalculator();
	bool kf_fix = true;
	const t_real* sensesCCW = nullptr;

	if(tascalc)
	{
		t_real kfix = std::get<0>(tascalc->GetKfix());
		kf_fix = std::get<1>(tascalc->GetKfix());
		sensesCCW = tascalc->GetScatteringSenses();

		if(kf_fix)
			ofstr << "kf(" << kfix << ")\n";
		else
			ofstr << "ki(" << kfix << ")\n";
	}

	ofstr << "\n# turn on air for entire path\n";
	ofstr << "move(\"air_sample\", 1)\n";
	if(kf_fix)
		ofstr << "move(\"air_mono\", 1)\n";
	else
		ofstr << "move(\"air_ana\", 1)\n";

	ofstr << "\n# disable motor backlash correction\n";
	ofstr << "stt_maxtries = stt.maxtries\n";
	ofstr << "stt.maxtries = 0\n";
	if(kf_fix)
	{
		ofstr << "mtt_maxtries = mtt.maxtries\n";
		ofstr << "mtt.maxtries = 0\n";
	}
	else
	{
		ofstr << "att_maxtries = att.maxtries\n";
		ofstr << "att.maxtries = 0\n";
	}

	// output motor drive commands
	ofstr << "\n# path vertices\n";
	for(const auto& vec : path)
	{
		t_real a4 = vec[0];
		t_real a2 = vec[1];

		if(path_in_rad)
		{
			a4 = a4 / tl2::pi<t_real> * t_real(180);
			a2 = a2 / tl2::pi<t_real> * t_real(180);
		}

		t_real sample_sense = 1.;
		if(sensesCCW)
			sample_sense = sensesCCW[1];
		ofstr << "maw(stt, " << a4*sample_sense << ", ";

		if(kf_fix)
		{
			t_real sense = 1.;
			if(sensesCCW)
				sense = sensesCCW[0];
			ofstr << "mtt, " << a2*sense << ")\n";
		}
		else
		{
			t_real sense = 1.;
			if(sensesCCW)
				sense = sensesCCW[2];
			ofstr << "att, " << a2*sense << ")\n";
		}
	}

	ofstr << "\n# turn off air\n";
	ofstr << "move(\"air_sample\", 0)\n";
	if(kf_fix)
		ofstr << "move(\"air_mono\", 0)\n";
	else
		ofstr << "move(\"air_ana\", 0)\n";

	ofstr << "\n# restore motor backlash correction\n";
	ofstr << "stt.maxtries = stt_maxtries\n";
	if(kf_fix)
		ofstr << "mtt.maxtries = mtt_maxtries\n";
	else
		ofstr << "att.maxtries = att_maxtries\n";

	ofstr.flush();
	return true;
}
