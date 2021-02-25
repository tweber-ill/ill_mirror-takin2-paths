/**
 * instrument walls
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __INSTR_H__
#define __INSTR_H__


#include "tlibs2/libs/file.h"
#include "globals.h"


class Instrument
{
public:
    Instrument();
    ~Instrument();

    void Clear();
    bool Load(const tl2::Prop<std::string>& prop, const std::string& basePath);


    t_real GetFloorLenX() const { return m_floorlen[0]; }
    t_real GetFloorLenY() const { return m_floorlen[1]; }


private:
    t_real m_floorlen[2] = { 10., 10. };
};


#endif
