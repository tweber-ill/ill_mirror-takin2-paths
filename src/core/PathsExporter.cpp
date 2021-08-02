/**
 * export paths to instrument control systems
 * @author Tobias Weber <tweber@ill.fr>
 * @date jul-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "PathsExporter.h"
#include "PathsBuilder.h"


/**
 * export the path as raw data
 */
bool PathsExporterRaw::Export(const PathsBuilder* builder) const
{
    return true;
}


/**
 * export the path into Nomad commands
 */
bool PathsExporterNomad::Export(const PathsBuilder* builder) const
{
    return true;
}


/**
 * export the path into Nicos commands
 */
bool PathsExporterNicos::Export(const PathsBuilder* builder) const
{
    return true;
}
