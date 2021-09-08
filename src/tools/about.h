/**
 * geo lib and tools
 * @author Tobias Weber <tweber@ill.fr>
 * @date nov-2020 - jun-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TAKIN_PATHS_TOOLS_ABOUT_H__
#define __TAKIN_PATHS_TOOLS_ABOUT_H__


#include <QtWidgets/QDialog>
#include <QtCore/QSettings>


class GeoAboutDlg : public QDialog
{
public:
	GeoAboutDlg(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~GeoAboutDlg();

	GeoAboutDlg(const GeoAboutDlg& other);
	const GeoAboutDlg& operator=(const GeoAboutDlg&);


protected:
	virtual void accept() override;


private:
	QSettings *m_sett{nullptr};
};


#endif
