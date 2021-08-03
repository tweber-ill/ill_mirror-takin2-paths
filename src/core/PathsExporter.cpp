/**
 * export paths to instrument control systems
 * @author Tobias Weber <tweber@ill.fr>
 * @date jul-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "PathsExporter.h"
#include "PathsBuilder.h"

#include <fstream>


/**
 * export the path as raw data
 */
bool PathsExporterRaw::Export(const PathsBuilder* builder, 
	const std::vector<t_vec>& path, bool path_in_rad) const
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

	for(const t_vec& vec : path)
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
	const std::vector<t_vec>& path, bool path_in_rad) const
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
	for(const t_vec& vec : path)
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
	const std::vector<t_vec>& path, bool path_in_rad) const
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
			ofstr << "kf(" << std::get<0>(kfix) << ")\n";
		else
			ofstr << "ki(" << std::get<0>(kfix) << ")\n";

		ofstr << "\n";
	}

	// output motor drive commands
	for(const t_vec& vec : path)
	{
		t_real a4 = vec[0];
		t_real a2 = vec[1];

		if(path_in_rad)
		{
			a4 = a4 / tl2::pi<t_real> * t_real(180);
			a2 = a2 / tl2::pi<t_real> * t_real(180);
		}

		ofstr << "stt(" << a4 << "); ";
		ofstr << "mtt(" << a2 << ");\n";
	}

	ofstr.flush();
	return true;
}