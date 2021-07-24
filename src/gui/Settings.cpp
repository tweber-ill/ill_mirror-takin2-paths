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
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHeaderView>

#include "tlibs2/libs/file.h"
#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/qt/numerictablewidgetitem.h"



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
enum class SettingsKeys : int
{
	EPS = 0,
	ANGULAR_EPS,
	GUI_EPS,

	PREC,
	GUI_PREC,

	MAX_THREADS,

	NUM_KEYS
};


SettingsDlg::SettingsDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Settings");

	// restore dialog geometry
	if(m_sett && m_sett->contains("settings/geo"))
		restoreGeometry(m_sett->value("settings/geo").toByteArray());
	else
		resize(512, 425);

	// create the settings table
	m_tab = new QTableWidget(this);
	m_tab->setShowGrid(true);
	m_tab->setSortingEnabled(false);
	m_tab->setMouseTracking(false);
	m_tab->setSelectionBehavior(QTableWidget::SelectRows);
	m_tab->setSelectionMode(QTableWidget::SingleSelection);

	// table headers
	m_tab->horizontalHeader()->setDefaultSectionSize(125);
	m_tab->verticalHeader()->setDefaultSectionSize(32);
	m_tab->verticalHeader()->setVisible(false);
	m_tab->setColumnCount(3);
	m_tab->setColumnWidth(0, 200);
	m_tab->setColumnWidth(1, 100);
	m_tab->setColumnWidth(2, 150);
	m_tab->setHorizontalHeaderItem(0, new QTableWidgetItem{"Key"});
	m_tab->setHorizontalHeaderItem(1, new QTableWidgetItem{"Type"});
	m_tab->setHorizontalHeaderItem(2, new QTableWidgetItem{"Value"});

	// table contents
	m_tab->setRowCount((int)SettingsKeys::NUM_KEYS);
	m_tab->setItem((int)SettingsKeys::EPS, 0, new QTableWidgetItem{"Calculation epsilon"});
	m_tab->setItem((int)SettingsKeys::EPS, 1, new QTableWidgetItem{"Real"});
	m_tab->setItem((int)SettingsKeys::EPS, 2, new NumericTableWidgetItem<t_real>(g_eps, 10));
	m_tab->setItem((int)SettingsKeys::ANGULAR_EPS, 0, new QTableWidgetItem{"Angular epsilon"});
	m_tab->setItem((int)SettingsKeys::ANGULAR_EPS, 1, new QTableWidgetItem{"Real"});
	m_tab->setItem((int)SettingsKeys::ANGULAR_EPS, 2, new NumericTableWidgetItem<t_real>(g_eps_angular, 10));
	m_tab->setItem((int)SettingsKeys::GUI_EPS, 0, new QTableWidgetItem{"Drawing epsilon"});
	m_tab->setItem((int)SettingsKeys::GUI_EPS, 1, new QTableWidgetItem{"Real"});
	m_tab->setItem((int)SettingsKeys::GUI_EPS, 2, new NumericTableWidgetItem<t_real>(g_eps_gui, 10));
	m_tab->setItem((int)SettingsKeys::PREC, 0, new QTableWidgetItem{"Number precision"});
	m_tab->setItem((int)SettingsKeys::PREC, 1, new QTableWidgetItem{"Integer"});
	m_tab->setItem((int)SettingsKeys::PREC, 2, new NumericTableWidgetItem<int>(g_prec, 10));
	m_tab->setItem((int)SettingsKeys::GUI_PREC, 0, new QTableWidgetItem{"GUI number precision"});
	m_tab->setItem((int)SettingsKeys::GUI_PREC, 1, new QTableWidgetItem{"Integer"});
	m_tab->setItem((int)SettingsKeys::GUI_PREC, 2, new NumericTableWidgetItem<int>(g_prec_gui, 10));
	m_tab->setItem((int)SettingsKeys::MAX_THREADS, 0, new QTableWidgetItem{"Maximum number of threads"});
	m_tab->setItem((int)SettingsKeys::MAX_THREADS, 1, new QTableWidgetItem{"Integer"});
	m_tab->setItem((int)SettingsKeys::MAX_THREADS, 2, new NumericTableWidgetItem<unsigned int>(g_maxnum_threads, 10));

	// set value field editable
	for(int row=0; row<m_tab->rowCount(); ++row)
	{
		m_tab->item(row, 0)->setFlags(m_tab->item(row, 0)->flags() & ~Qt::ItemIsEditable);
		m_tab->item(row, 1)->setFlags(m_tab->item(row, 1)->flags() & ~Qt::ItemIsEditable);
		m_tab->item(row, 2)->setFlags(m_tab->item(row, 2)->flags() | Qt::ItemIsEditable);
	}


	auto grid = new QGridLayout(this);
	grid->setSpacing(4);
	grid->setContentsMargins(12, 12, 12, 12);

	int y = 0;
	grid->addWidget(m_tab, y++,0,1,1);

	//QSpacerItem *spacer_end = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
	//grid->addItem(spacer_end, y++,0,1,1);

	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
	grid->addWidget(buttons, y++,0,1,1);

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(buttons, &QDialogButtonBox::clicked, [this, buttons](QAbstractButton* btn)
	{
		// apply button was pressed
		if(btn == static_cast<QAbstractButton*>(buttons->button(QDialogButtonBox::Apply)))
			ApplySettings();
	});
}


SettingsDlg::~SettingsDlg()
{
}


template<class T>
static void get_setting(QSettings* sett, const char* key, T* val)
{
	if(sett->contains(key))
	{
		*val = sett->value(key, *val).template value<T>();
		//std::cout << key << ": " << *val << std::endl;
	}
}


/**
 * read the settings and set the global variables 
 */
void SettingsDlg::ReadSettings(QSettings* sett)
{
	if(!sett)
		return;

	get_setting<t_real>(sett, "settings/eps", &g_eps);
	get_setting<t_real>(sett, "settings/eps_angular", &g_eps_angular);
	get_setting<t_real>(sett, "settings/eps_gui", &g_eps_gui);

	get_setting<int>(sett, "settings/prec", &g_prec);
	get_setting<int>(sett, "settings/prec_gui", &g_prec_gui);

	get_setting<unsigned int>(sett, "settings/maxnum_threads", &g_maxnum_threads);
}


/**
 * 'Apply' was clicked, write the settings from the global variables
 */
void SettingsDlg::ApplySettings()
{
	// set the global variables
	g_eps = dynamic_cast<NumericTableWidgetItem<t_real>*>(
		m_tab->item((int)SettingsKeys::EPS, 2))->GetValue();
	g_eps_angular = dynamic_cast<NumericTableWidgetItem<t_real>*>(
		m_tab->item((int)SettingsKeys::ANGULAR_EPS, 2))->GetValue();
	g_eps_gui = dynamic_cast<NumericTableWidgetItem<t_real>*>(
		m_tab->item((int)SettingsKeys::GUI_EPS, 2))->GetValue();

	g_prec = dynamic_cast<NumericTableWidgetItem<int>*>(
		m_tab->item((int)SettingsKeys::PREC, 2))->GetValue();
	g_prec_gui = dynamic_cast<NumericTableWidgetItem<int>*>(
		m_tab->item((int)SettingsKeys::GUI_PREC, 2))->GetValue();

	g_maxnum_threads = dynamic_cast<NumericTableWidgetItem<unsigned int>*>(
		m_tab->item((int)SettingsKeys::MAX_THREADS, 2))->GetValue();


	// write out the settings
	if(m_sett)
	{
		m_sett->setValue("settings/eps", g_eps);
		m_sett->setValue("settings/eps_angular", g_eps_angular);
		m_sett->setValue("settings/eps_gui", g_eps_gui);

		m_sett->setValue("settings/prec", g_prec);
		m_sett->setValue("settings/prec_gui", g_prec_gui);

		m_sett->setValue("settings/maxnum_threads", g_maxnum_threads);
	}
}


/**
 * 'OK' was clicked
 */
void SettingsDlg::accept()
{
	ApplySettings();

	if(m_sett)
		m_sett->setValue("settings/geo", saveGeometry());
	QDialog::accept();
}
// ----------------------------------------------------------------------------
