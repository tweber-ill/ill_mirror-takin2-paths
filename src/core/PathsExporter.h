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

#ifndef __GEO_PATHS_EXPORTER_H__
#define __GEO_PATHS_EXPORTER_H__


#include <string>
#include "types.h"


class PathsBuilder;


enum class PathsExporterFormat
{
	RAW,
	NOMAD,
	NICOS
};


/**
 * base class for exporter visitor
 */
class PathsExporterBase
{
public:
	virtual ~PathsExporterBase() = default;
	virtual bool Export(const PathsBuilder* builder, const std::vector<t_vec2>& path, 
		bool path_in_rad = false) const = 0;
};


/**
 * export raw data points
 */
class PathsExporterRaw : public PathsExporterBase
{
public:
	PathsExporterRaw(const std::string& filename) : m_filename(filename) {};
	virtual ~PathsExporterRaw() = default;

	virtual bool Export(const PathsBuilder* builder, const std::vector<t_vec2>& path, 
		bool path_in_rad = false) const override;

private:
	std::streamsize m_prec = 6;
	std::string m_filename;
};


/**
 * export to nomad
 */
class PathsExporterNomad : public PathsExporterBase
{
public:
	PathsExporterNomad(const std::string& filename) : m_filename(filename) {};
	virtual ~PathsExporterNomad() = default;

	virtual bool Export(const PathsBuilder* builder, const std::vector<t_vec2>& path, 
		bool path_in_rad = false) const override;

private:
	std::streamsize m_prec = 6;
	std::string m_filename;
};


/**
 * export to nicos
 */
class PathsExporterNicos : public PathsExporterBase
{
public:
	PathsExporterNicos(const std::string& filename) : m_filename(filename) {}
	virtual ~PathsExporterNicos() = default;

	virtual bool Export(const PathsBuilder* builder, const std::vector<t_vec2>& path, 
		bool path_in_rad = false) const override;

private:
	std::streamsize m_prec = 6;
	std::string m_filename;
};


#endif
