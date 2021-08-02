/**
 * export paths to instrument control systems
 * @author Tobias Weber <tweber@ill.fr>
 * @date jul-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __GEO_PATHS_EXPORTER_H__
#define __GEO_PATHS_EXPORTER_H__


class PathsBuilder;


/**
 * base class for exporter visitor
 */
class PathsExporterBase
{
public:
	virtual void Export(const PathsBuilder* builder) const = 0;
};


/**
 * export to nomad
 */
class PathsExporterNomad : public PathsExporterBase
{
public:
	virtual void Export(const PathsBuilder* builder) const override;
};


/**
 * export to nicos
 */
class PathsExporterNicos : public PathsExporterBase
{
public:
	virtual void Export(const PathsBuilder* builder) const override;
};


#endif
