/**
 * export paths to instrument control systems
 * @author Tobias Weber <tweber@ill.fr>
 * @date jul-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __GEO_PATHS_EXPORTER_H__
#define __GEO_PATHS_EXPORTER_H__


#include <string>


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
	virtual bool Export(const PathsBuilder* builder) const = 0;
};


/**
 * export raw data points
 */
class PathsExporterRaw : public PathsExporterBase
{
public:
	PathsExporterRaw(const std::string& filename) : m_filename(filename) {};
	virtual bool Export(const PathsBuilder* builder) const override;

private:
	std::string m_filename;
};


/**
 * export to nomad
 */
class PathsExporterNomad : public PathsExporterBase
{
public:
	PathsExporterNomad(const std::string& filename) : m_filename(filename) {};
	virtual bool Export(const PathsBuilder* builder) const override;

private:
	std::string m_filename;
};


/**
 * export to nicos
 */
class PathsExporterNicos : public PathsExporterBase
{
public:
	PathsExporterNicos(const std::string& filename) : m_filename(filename) {}
	virtual bool Export(const PathsBuilder* builder) const override;

private:
	std::string m_filename;
};


#endif
