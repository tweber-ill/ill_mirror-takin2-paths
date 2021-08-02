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
bool PathsExporterRaw::Export(const PathsBuilder* builder, const std::vector<t_vec>& path) const
{
	if(!builder)
		return false;

	std::ofstream ofstr(m_filename);
	if(!ofstr)
		return false;

	ofstr.precision(m_prec);

	ofstr << "# "
		<< std::right << std::setw(m_prec*2-2) << "a4 (deg)" << " "
		<< std::right << std::setw(m_prec*2) << "a2 (deg)" << "\n";

	for(const t_vec& vec : path)
	{
		ofstr
			<< std::right << std::setw(m_prec*2) << vec[0] << " "
			<< std::right << std::setw(m_prec*2) << vec[1] << "\n";
	}

	ofstr.flush();
    return true;
}


/**
 * export the path into Nomad commands
 */
bool PathsExporterNomad::Export(const PathsBuilder* builder, const std::vector<t_vec>& path) const
{
	if(!builder)
		return false;

	std::ofstream ofstr(m_filename);
	if(!ofstr)
		return false;

	ofstr.precision(m_prec);

	for(const t_vec& vec : path)
	{
		ofstr
			<< "dr a4 " << std::left << std::setw(m_prec*2) << vec[0] << " "
			<< "a2 " << std::left << std::setw(m_prec*2) << vec[1] << "\n";
	}

	ofstr.flush();
    return true;
}


/**
 * export the path into Nicos commands
 */
bool PathsExporterNicos::Export(const PathsBuilder* builder, const std::vector<t_vec>& path) const
{
	if(!builder)
		return false;

	std::ofstream ofstr(m_filename);
	if(!ofstr)
		return false;

	ofstr.precision(m_prec);

	for(const t_vec& vec : path)
	{
		ofstr << "stt(" << vec[0] << "); ";
		ofstr << "mtt(" << vec[1] << ");\n";
	}

	ofstr.flush();
	return true;
}
