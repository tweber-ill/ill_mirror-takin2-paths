/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "Settings.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QDialogButtonBox>

#include "tlibs2/libs/file.h"
#include "tlibs2/libs/maths.h"


// ----------------------------------------------------------------------------
// global settings variables
// ----------------------------------------------------------------------------
std::string g_apppath = ".";

unsigned int g_timer_fps = 30;

tl2::t_real_gl g_move_scale = tl2::t_real_gl(1./75.);
tl2::t_real_gl g_rotation_scale = 0.02;

int g_prec = 6;
int g_prec_gui = 4;

t_real g_eps = 1e-6;
t_real g_eps_angular = 0.01/180.*tl2::pi<t_real>;
t_real g_eps_gui = 1e-4;

t_real g_a3_offs = tl2::pi<t_real>*0.5;

unsigned int g_maxnum_threads = 4;
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------
/**
 * get the path to a resource file
 */
std::string find_resource(const std::string& resfile)
{
	fs::path res = resfile;
	fs::path apppath = g_apppath;

	// iterate possible resource directories
	for(const fs::path& path :
	{
		apppath/"res"/res, apppath/".."/"res"/res,
		apppath/"Resources"/res, apppath/".."/"Resources"/res,
		fs::path("/usr/local/share/TASPaths/res")/res, fs::path("/usr/share/TASPaths/res")/res,
	})
	{
		if(fs::exists(path))
			return path.string();
	}

	return "";
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// settings dialog
// ----------------------------------------------------------------------------
SettingsDlg::SettingsDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Settings");

	// restore dialog geometry
	if(m_sett && m_sett->contains("settings/geo"))
		restoreGeometry(m_sett->value("settings/geo").toByteArray());

	auto grid = new QGridLayout(this);
	grid->setSpacing(4);
	grid->setContentsMargins(12, 12, 12, 12);

	int y = 0;

	QSpacerItem *spacer_end = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
	grid->addItem(spacer_end, y++,0,1,1);

	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Ok);
	grid->addWidget(buttons, y++,0,1,1);

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
}


SettingsDlg::~SettingsDlg()
{
}


void SettingsDlg::accept()
{
	if(m_sett)
		m_sett->setValue("settings/geo", saveGeometry());
	QDialog::accept();
}
// ----------------------------------------------------------------------------
