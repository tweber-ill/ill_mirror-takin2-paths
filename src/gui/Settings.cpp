/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "Settings.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QFontDialog>

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

t_real g_a2_delta = 0.5 / 180. * tl2::pi<t_real>;
t_real g_a4_delta = 1. / 180. * tl2::pi<t_real>;

unsigned int g_maxnum_threads = 4;

QString g_theme = "";
QString g_font = "";
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

	A3_OFFS,
	A2_DELTA,
	A4_DELTA,

	NUM_KEYS
};


template<class T>
static void get_setting(QSettings* sett, const char* key, T* val)
{
	if(sett->contains(key))
	{
		*val = sett->value(key, *val).template value<T>();
		//std::cout << key << ": " << *val << std::endl;
	}
}


SettingsDlg::SettingsDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Settings");

	// restore dialog geometry
	if(m_sett && m_sett->contains("settings/geo"))
		restoreGeometry(m_sett->value("settings/geo").toByteArray());
	else
		resize(512, 425);

	// general settings
	QWidget *panelGeneral = new QWidget(this);
	QGridLayout *gridGeneral = new QGridLayout(panelGeneral);
	gridGeneral->setSpacing(4);
	gridGeneral->setContentsMargins(6, 6, 6, 6);

	// create the settings table
	m_table = new QTableWidget(panelGeneral);
	m_table->setShowGrid(true);
	m_table->setSortingEnabled(false);
	m_table->setMouseTracking(false);
	m_table->setSelectionBehavior(QTableWidget::SelectRows);
	m_table->setSelectionMode(QTableWidget::SingleSelection);

	// table headers
	m_table->horizontalHeader()->setDefaultSectionSize(125);
	m_table->verticalHeader()->setDefaultSectionSize(32);
	m_table->verticalHeader()->setVisible(false);
	m_table->setColumnCount(3);
	m_table->setColumnWidth(0, 200);
	m_table->setColumnWidth(1, 100);
	m_table->setColumnWidth(2, 150);
	m_table->setHorizontalHeaderItem(0, new QTableWidgetItem{"Key"});
	m_table->setHorizontalHeaderItem(1, new QTableWidgetItem{"Type"});
	m_table->setHorizontalHeaderItem(2, new QTableWidgetItem{"Value"});


	// table contents
	m_table->setRowCount((int)SettingsKeys::NUM_KEYS);

	m_table->setItem((int)SettingsKeys::EPS, 0, new QTableWidgetItem{"Calculation epsilon"});
	m_table->setItem((int)SettingsKeys::EPS, 1, new QTableWidgetItem{"Real"});
	m_table->setItem((int)SettingsKeys::EPS, 2, new NumericTableWidgetItem<t_real>(g_eps, 10));

	m_table->setItem((int)SettingsKeys::ANGULAR_EPS, 0, new QTableWidgetItem{"Angular epsilon"});
	m_table->setItem((int)SettingsKeys::ANGULAR_EPS, 1, new QTableWidgetItem{"Real"});
	m_table->setItem((int)SettingsKeys::ANGULAR_EPS, 2, new NumericTableWidgetItem<t_real>(g_eps_angular, 10));

	m_table->setItem((int)SettingsKeys::GUI_EPS, 0, new QTableWidgetItem{"Drawing epsilon"});
	m_table->setItem((int)SettingsKeys::GUI_EPS, 1, new QTableWidgetItem{"Real"});
	m_table->setItem((int)SettingsKeys::GUI_EPS, 2, new NumericTableWidgetItem<t_real>(g_eps_gui, 10));

	m_table->setItem((int)SettingsKeys::PREC, 0, new QTableWidgetItem{"Number precision"});
	m_table->setItem((int)SettingsKeys::PREC, 1, new QTableWidgetItem{"Integer"});
	m_table->setItem((int)SettingsKeys::PREC, 2, new NumericTableWidgetItem<int>(g_prec, 10));

	m_table->setItem((int)SettingsKeys::GUI_PREC, 0, new QTableWidgetItem{"GUI number precision"});
	m_table->setItem((int)SettingsKeys::GUI_PREC, 1, new QTableWidgetItem{"Integer"});
	m_table->setItem((int)SettingsKeys::GUI_PREC, 2, new NumericTableWidgetItem<int>(g_prec_gui, 10));

	m_table->setItem((int)SettingsKeys::MAX_THREADS, 0, new QTableWidgetItem{"Maximum number of threads"});
	m_table->setItem((int)SettingsKeys::MAX_THREADS, 1, new QTableWidgetItem{"Integer"});
	m_table->setItem((int)SettingsKeys::MAX_THREADS, 2, new NumericTableWidgetItem<unsigned int>(g_maxnum_threads, 10));

	m_table->setItem((int)SettingsKeys::A3_OFFS, 0, new QTableWidgetItem{"Sample rotation offset"});
	m_table->setItem((int)SettingsKeys::A3_OFFS, 1, new QTableWidgetItem{"Real"});
	m_table->setItem((int)SettingsKeys::A3_OFFS, 2, new NumericTableWidgetItem<t_real>(g_a3_offs / tl2::pi<t_real>*180., 10));

	m_table->setItem((int)SettingsKeys::A2_DELTA, 0, new QTableWidgetItem{"Monochromator scattering angle delta"});
	m_table->setItem((int)SettingsKeys::A2_DELTA, 1, new QTableWidgetItem{"Real"});
	m_table->setItem((int)SettingsKeys::A2_DELTA, 2, new NumericTableWidgetItem<t_real>(g_a2_delta / tl2::pi<t_real>*180., 10));

	m_table->setItem((int)SettingsKeys::A4_DELTA, 0, new QTableWidgetItem{"Sample scattering angle delta"});
	m_table->setItem((int)SettingsKeys::A4_DELTA, 1, new QTableWidgetItem{"Real"});
	m_table->setItem((int)SettingsKeys::A4_DELTA, 2, new NumericTableWidgetItem<t_real>(g_a4_delta / tl2::pi<t_real>*180., 10));


	// set value field editable
	for(int row=0; row<m_table->rowCount(); ++row)
	{
		m_table->item(row, 0)->setFlags(m_table->item(row, 0)->flags() & ~Qt::ItemIsEditable);
		m_table->item(row, 1)->setFlags(m_table->item(row, 1)->flags() & ~Qt::ItemIsEditable);
		m_table->item(row, 2)->setFlags(m_table->item(row, 2)->flags() | Qt::ItemIsEditable);
	}

	gridGeneral->addWidget(m_table, 0,0,1,1);


	// gui settings
	QWidget *panelGui = new QWidget(this);
	QGridLayout *gridGui = new QGridLayout(panelGui);
	gridGui->setSpacing(4);
	gridGui->setContentsMargins(6, 6, 6, 6);
	int yGui = 0;

	QLabel *labelTheme = new QLabel("Style:", panelGui);
	labelTheme->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	m_comboTheme = new QComboBox(panelGui);
	m_comboTheme->addItems(QStyleFactory::keys());

	get_setting<QString>(sett, "settings/theme", &g_theme);
	if(g_theme != "")
	{
		int idxTheme = m_comboTheme->findText(g_theme);
		if(idxTheme >= 0 && idxTheme < m_comboTheme->count())
			m_comboTheme->setCurrentIndex(idxTheme);
	}

	QLabel *labelFont = new QLabel("Font:", panelGui);
	labelFont->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	m_editFont = new QLineEdit(panelGui);
	m_editFont->setReadOnly(true);
	QPushButton *btnFont = new QPushButton("Select...", panelGui);

	get_setting<QString>(sett, "settings/font", &g_font);
	if(g_font == "")
		g_font = QApplication::font().toString();
	m_editFont->setText(g_font);

	gridGui->addWidget(labelTheme, yGui,0,1,1);
	gridGui->addWidget(m_comboTheme, yGui++,1,1,2);
	gridGui->addWidget(labelFont, yGui,0,1,1);
	gridGui->addWidget(m_editFont, yGui,1,1,1);
	gridGui->addWidget(btnFont, yGui++,2,1,1);

	QSpacerItem *spacer_end = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
	gridGui->addItem(spacer_end, yGui++,0,1,3);


	// main grid
	QGridLayout *grid = new QGridLayout(this);
	grid->setSpacing(4);
	grid->setContentsMargins(12, 12, 12, 12);
	int y = 0;

	QTabWidget *tab = new QTabWidget(this);
	tab->addTab(panelGeneral, "General");
	tab->addTab(panelGui, "GUI");
	grid->addWidget(tab, y++,0,1,1);

	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
	grid->addWidget(buttons, y++,0,1,1);


	// connections
	connect(btnFont, &QPushButton::clicked, [this]()
	{
		// current font
		QFont font = QApplication::font();

		// select a new font
		bool okClicked = false;
		font = QFontDialog::getFont(&okClicked, font, this);
		if(okClicked)
		{
			g_font = font.toString();
			if(g_font == "")
				g_font = QApplication::font().toString();
			m_editFont->setText(g_font);

			//QApplication::setFont(font);
		}

		// hack for the QFontDialog hiding the settings dialog
		this->show();
		this->raise();
		this->activateWindow();
	});

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

	get_setting<QString>(sett, "settings/theme", &g_theme);
	get_setting<QString>(sett, "settings/font", &g_font);

	get_setting<t_real>(sett, "settings/a3_offs", &g_a3_offs);
	get_setting<t_real>(sett, "settings/a2_delta", &g_a2_delta);
	get_setting<t_real>(sett, "settings/a4_delta", &g_a4_delta);

	ApplyGuiSettings();
}


/**
 * 'Apply' was clicked, write the settings from the global variables
 */
void SettingsDlg::ApplySettings()
{
	// set the global variables
	g_eps = dynamic_cast<NumericTableWidgetItem<t_real>*>(
		m_table->item((int)SettingsKeys::EPS, 2))->GetValue();
	g_eps_angular = dynamic_cast<NumericTableWidgetItem<t_real>*>(
		m_table->item((int)SettingsKeys::ANGULAR_EPS, 2))->GetValue();
	g_eps_gui = dynamic_cast<NumericTableWidgetItem<t_real>*>(
		m_table->item((int)SettingsKeys::GUI_EPS, 2))->GetValue();

	g_prec = dynamic_cast<NumericTableWidgetItem<int>*>(
		m_table->item((int)SettingsKeys::PREC, 2))->GetValue();
	g_prec_gui = dynamic_cast<NumericTableWidgetItem<int>*>(
		m_table->item((int)SettingsKeys::GUI_PREC, 2))->GetValue();

	g_maxnum_threads = dynamic_cast<NumericTableWidgetItem<unsigned int>*>(
		m_table->item((int)SettingsKeys::MAX_THREADS, 2))->GetValue();

	g_a3_offs = dynamic_cast<NumericTableWidgetItem<t_real>*>(
		m_table->item((int)SettingsKeys::A3_OFFS, 2))->GetValue()
			/ 180.*tl2::pi<t_real>;

	g_a2_delta = dynamic_cast<NumericTableWidgetItem<t_real>*>(
		m_table->item((int)SettingsKeys::A2_DELTA, 2))->GetValue()
			/ 180.*tl2::pi<t_real>;

	g_a4_delta = dynamic_cast<NumericTableWidgetItem<t_real>*>(
		m_table->item((int)SettingsKeys::A4_DELTA, 2))->GetValue()
			/ 180.*tl2::pi<t_real>;

	g_theme = m_comboTheme->currentText();
	g_font = m_editFont->text();


	// write out the settings
	if(m_sett)
	{
		m_sett->setValue("settings/eps", g_eps);
		m_sett->setValue("settings/eps_angular", g_eps_angular);
		m_sett->setValue("settings/eps_gui", g_eps_gui);

		m_sett->setValue("settings/prec", g_prec);
		m_sett->setValue("settings/prec_gui", g_prec_gui);

		m_sett->setValue("settings/maxnum_threads", g_maxnum_threads);

		m_sett->setValue("settings/a3_offs", g_a3_offs);
		m_sett->setValue("settings/a2_delta", g_a2_delta);
		m_sett->setValue("settings/a4_delta", g_a4_delta);

		m_sett->setValue("settings/theme", g_theme);
		m_sett->setValue("settings/font", g_font);
	}

	ApplyGuiSettings();
}


void SettingsDlg::ApplyGuiSettings()
{
	// set gui theme
	if(g_theme != "")
	{
		if(QStyle* theme = QStyleFactory::create(g_theme); theme)
			QApplication::setStyle(theme);
	}

	// set gui font
	if(g_font != "")
	{
		QFont font;
		if(font.fromString(g_font))
			QApplication::setFont(font);
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
