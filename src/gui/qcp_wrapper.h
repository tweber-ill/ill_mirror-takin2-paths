/**
 * helper to get rid of qcustomplot warnings
 * @author Tobias Weber <tweber@ill.fr>
 * @date sep-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __QCP_WARPPER_H__
#define __QCP_WARPPER_H__

// get rid of certain warnings for external files
// see: https://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Pragmas.html
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
	#include "qcustomplot/qcustomplot.h"
#pragma GCC diagnostic pop

#endif
