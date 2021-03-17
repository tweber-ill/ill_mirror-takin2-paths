/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TAKIN_PATHS_ABOUT_H__
#define __TAKIN_PATHS_ABOUT_H__


#include <QtWidgets/QFileDialog>


class AboutDlg : public QDialog
{
    public:
        AboutDlg(QWidget* parent=nullptr);
        virtual ~AboutDlg();
};


#endif
