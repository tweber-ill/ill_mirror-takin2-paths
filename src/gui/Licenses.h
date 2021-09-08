/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date sep-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TAKIN_PATHS_LICENSES_H__
#define __TAKIN_PATHS_LICENSES_H__


#include <QtWidgets/QDialog>
#include <QtCore/QSettings>


class LicensesDlg : public QDialog
{
public:
	LicensesDlg(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~LicensesDlg();

	LicensesDlg(const LicensesDlg& other);
	LicensesDlg& operator=(const LicensesDlg&);


protected:
	virtual void accept() override;


private:
	QSettings *m_sett{nullptr};
};


#endif
