/**
 * instrument walls
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "Instrument.h"


Instrument::Instrument()
{
}


Instrument::~Instrument()
{
}


void Instrument::Clear()
{
    // reset to defaults
    m_floorlen[0] = m_floorlen[1] = 10.;
}


/**
 * load instrument and wall configuration
 */
bool Instrument::Load(const tl2::Prop<std::string>& prop, const std::string& basePath)
{
    Clear();

    // floor size
    if(auto optFloorLenX = prop.QueryOpt<t_real>(basePath + "floor_len_x"); optFloorLenX)
        m_floorlen[0] = *optFloorLenX;
    if(auto optFloorLenY = prop.QueryOpt<t_real>(basePath + "floor_len_y"); optFloorLenY)
        m_floorlen[1] = *optFloorLenY;

    return true;
}
