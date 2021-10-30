/**
 * TAS paths tool -- settings dialog and resource management
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite)
 * Copyright (C) 2021  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                     Grenoble, France).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 */

#include "Settings.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QFontDialog>

#include <array>
#include <type_traits>

#include "tlibs2/libs/file.h"
#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/qt/numerictablewidgetitem.h"



// ----------------------------------------------------------------------------
// global settings variables
// ----------------------------------------------------------------------------
std::string g_apppath = ".";


// maximum number of threads
unsigned int g_maxnum_threads = 4;


// epsilons and precisions
int g_prec = 6;
int g_prec_gui = 4;
t_real g_eps = 1e-6;
t_real g_eps_angular = 0.01 / 180. * tl2::pi<t_real>;
t_real g_eps_gui = 1e-4;
t_real g_eps_voronoiedge = 2e-2;

t_real g_line_subdiv_len = 0.025;

t_real g_a3_offs = tl2::pi<t_real>*0.5;

t_real g_a2_delta = 0.5 / 180. * tl2::pi<t_real>;
t_real g_a4_delta = 1. / 180. * tl2::pi<t_real>;


// which polygon intersection method should be used?
// 0: sweep, 1: half-plane test
int g_poly_intersection_method = 1;

// which backend to use for voronoi diagram calculation?
// 0: boost.polygon, 1: cgal
int g_voronoi_backend = 0;

// use region calculation function
int g_use_region_function = 1;


// path-finding options
int g_pathstrategy = 0;
int g_verifypath = 1;


// path-tracker and renderer FPS
unsigned int g_pathtracker_fps = 30;
unsigned int g_timer_fps = 30;


// renderer options
tl2::t_real_gl g_move_scale = tl2::t_real_gl(1./75.);
tl2::t_real_gl g_rotation_scale = tl2::t_real_gl(0.02);

int g_light_follows_cursor = 0;
int g_enable_shadow_rendering = 1;

// screenshots
int g_combined_screenshots = 0;
int g_automatic_screenshots = 0;


// gui theme
QString g_theme = "";

// gui font
QString g_font = "";

// use native menubar?
int g_use_native_menubar = 0;

// use native dialogs?
int g_use_native_dialogs = 1;
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// variables register
// ----------------------------------------------------------------------------
struct SettingsVariable
{
	const char* description{};
	const char* key{};
	std::variant<t_real*, int*, unsigned int*> value{};
	bool is_angle{false};
};

static constexpr std::array<SettingsVariable, 22> g_settingsvariables
{{
	// epsilons and precisions
	{.description = "Calculation epsilon", .key = "settings/eps", .value = &g_eps,},
	{.description = "Angular epsilon", .key = "settings/eps_angular", .value = &g_eps_angular, .is_angle = true},
	{.description = "Voronoi edge epsilon", .key = "settings/eps_voronoi_edge", .value = &g_eps_voronoiedge},
	{.description = "Drawing epsilon", .key = "settings/eps_gui", .value = &g_eps_gui},
	{.description = "Number precision", .key = "settings/prec", .value = &g_prec},
	{.description = "GUI number precision", .key = "settings/prec_gui", .value = &g_prec_gui},

	{.description = "Line subdivision length", .key = "settings/line_subdiv_len", .value = &g_line_subdiv_len},

	// threading options
	{.description = "Maximum number of threads", .key = "settings/maxnum_threads", .value = &g_maxnum_threads},

	// angle options
	{.description = "Sample rotation offset", .key = "settings/a3_offs", .value = &g_a3_offs, .is_angle = true},
	{.description = "Monochromator scattering angle delta", .key = "settings/a2_delta", .value = &g_a2_delta, .is_angle = true},
	{.description = "Sample scattering angle delta", .key = "settings/a4_delta", .value = &g_a4_delta, .is_angle = true},

	// mesh options
	{.description = "Polygon intersection method", .key = "settings/poly_inters_method", .value = &g_poly_intersection_method},
	{.description = "Voronoi calculation backend", .key = "settings/voronoi_backend", .value = &g_voronoi_backend},
	{.description = "Use region function", .key = "settings/use_region_function", .value = &g_use_region_function},

	// path options
	{.description = "Path finding strategy", .key = "settings/path_finding_strategy", .value = &g_pathstrategy},
	{.description = "Verify generated path", .key = "settings/verify_path", .value = &g_verifypath},
	{.description = "Path tracker FPS", .key = "settings/pathtracker_fps", .value = &g_pathtracker_fps},

	// renderer options
	{.description = "Renderer FPS", .key = "settings/renderer_fps", .value = &g_timer_fps},
	{.description = "Light follows cursor", .key = "settings/light_follows_cursor", .value = &g_light_follows_cursor},
	{.description = "Enable shadow rendering", .key = "settings/enable_shadow_rendering", .value = &g_enable_shadow_rendering},

	// screenshot options
	{.description = "Combine instrument/configuration space screenshots", .key = "settings/combined_screenshots", .value = &g_combined_screenshots},
	{.description = "Automatically take screenshots (careful!)", .key = "settings/automatic_screenshots", .value = &g_automatic_screenshots},
}};
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
		apppath/res, apppath/".."/res,
		fs::path("/usr/local/share/TASPaths/res")/res, fs::path("/usr/share/TASPaths/res")/res,
		fs::path("/usr/local/share/TASPaths")/res, fs::path("/usr/share/TASPaths")/res,
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

// type names
template<class T> const char* type_str = "Unknown";
template<> const char* type_str<t_real> = "Real";
template<> const char* type_str<int> = "Integer";
template<> const char* type_str<unsigned int> = "Integer, unsigned";


/**
 * adds a settings item from a global variable to the table
 */
template<std::size_t idx>
static void add_table_item(QTableWidget *table)
{
	constexpr const SettingsVariable& var = std::get<idx>(g_settingsvariables);
	constexpr const auto* value = std::get<var.value.index()>(var.value);
	using t_value = std::decay_t<decltype(*value)>;

	t_value finalval = *value;
	if(var.is_angle)
		finalval = finalval / tl2::pi<t_real>*180;

	table->setItem((int)idx, 0, new QTableWidgetItem{var.description});
	table->setItem((int)idx, 1, new QTableWidgetItem{type_str<t_value>});
	table->setItem((int)idx, 2, new NumericTableWidgetItem<t_value>(finalval, 10));
}


/**
 * adds all settings items from the global variables to the table
 */
template<std::size_t ...seq>
static constexpr void add_table_item_loop(
	const std::index_sequence<seq...>&, QTableWidget *table)
{
	// a sequence of function calls
	( (add_table_item<seq>(table)), ... );
}


/**
 * get a value from a QSettings object
 */
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
 * gets a settings item from the QSettings object and saves it
 * to the corresponding global variable
 */
template<std::size_t idx>
static void get_settings_item(QSettings *sett)
{
	constexpr const SettingsVariable& var = std::get<idx>(g_settingsvariables);
	constexpr auto* value = std::get<var.value.index()>(var.value);
	using t_value = std::decay_t<decltype(*value)>;

	get_setting<t_value>(sett, var.key, value);
}


/**
 * gets all settings items from the QSettings object and saves them
 * to the global variables
 */
template<std::size_t ...seq>
static constexpr void get_settings_loop(
	const std::index_sequence<seq...>&, QSettings *sett)
{
	// a sequence of function calls
	( (get_settings_item<seq>(sett)), ... );
}


/**
 * reads a settings item from the table and saves it to the
 * corresponding global variable and to the QSettings object
 */
template<std::size_t idx>
static void apply_settings_item(QTableWidget* table, QSettings *sett)
{
	constexpr const SettingsVariable& var = std::get<idx>(g_settingsvariables);
	constexpr auto* value = std::get<var.value.index()>(var.value);
	using t_value = std::decay_t<decltype(*value)>;

	t_value finalval = dynamic_cast<NumericTableWidgetItem<t_value>*>(
		table->item((int)idx, 2))->GetValue();
	if(var.is_angle)
		finalval = finalval / 180.*tl2::pi<t_real>;

	// set the global variable
	*value = finalval;

	// write out the settings
	if(sett)
		sett->setValue(var.key, *value);
}


/**
 * reads all settings items from the table and saves them to the
 * global variables and to the QSettings object
 */
template<std::size_t ...seq>
static constexpr void apply_settings_loop(
	const std::index_sequence<seq...>&, QTableWidget* table, QSettings *sett)
{
	// a sequence of function calls
	( (apply_settings_item<seq>(table, sett)), ... );
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
	m_table->setRowCount((int)g_settingsvariables.size());

	auto seq = std::make_index_sequence<g_settingsvariables.size()>();
	add_table_item_loop(seq, m_table);

	// set value field editable
	for(int row=0; row<m_table->rowCount(); ++row)
	{
		m_table->item(row, 0)->setFlags(m_table->item(row, 0)->flags() & ~Qt::ItemIsEditable);
		m_table->item(row, 1)->setFlags(m_table->item(row, 1)->flags() & ~Qt::ItemIsEditable);
		m_table->item(row, 2)->setFlags(m_table->item(row, 2)->flags() | Qt::ItemIsEditable);
	}

	// search field
	QLabel *labelSearch = new QLabel("Search:", panelGeneral);
	QLineEdit *editSearch = new QLineEdit(panelGeneral);

	labelSearch->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	editSearch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	gridGeneral->addWidget(m_table, 0, 0, 1, 2);
	gridGeneral->addWidget(labelSearch, 1, 0, 1, 1);
	gridGeneral->addWidget(editSearch, 1, 1, 1, 1);


	// gui settings
	QWidget *panelGui = new QWidget(this);
	QGridLayout *gridGui = new QGridLayout(panelGui);
	gridGui->setSpacing(4);
	gridGui->setContentsMargins(6, 6, 6, 6);
	int yGui = 0;

	// theme
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

	// font
	QLabel *labelFont = new QLabel("Font:", panelGui);
	labelFont->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	m_editFont = new QLineEdit(panelGui);
	m_editFont->setReadOnly(true);
	QPushButton *btnFont = new QPushButton("Select...", panelGui);

	get_setting<QString>(sett, "settings/font", &g_font);
	if(g_font == "")
		g_font = QApplication::font().toString();
	m_editFont->setText(g_font);

	// native menubar
	m_checkMenubar = new QCheckBox("Use native menubar", panelGui);
	get_setting<decltype(g_use_native_menubar)>(sett, "settings/native_menubar", &g_use_native_menubar);
	m_checkMenubar->setChecked(g_use_native_menubar!=0);

	// native dialogs
	m_checkDialogs = new QCheckBox("Use native dialogs", panelGui);
	get_setting<decltype(g_use_native_dialogs)>(sett, "settings/native_dialogs", &g_use_native_dialogs);
	m_checkDialogs->setChecked(g_use_native_dialogs!=0);

	// add widgets to layout
	gridGui->addWidget(labelTheme, yGui,0,1,1);
	gridGui->addWidget(m_comboTheme, yGui++,1,1,2);
	gridGui->addWidget(labelFont, yGui,0,1,1);
	gridGui->addWidget(m_editFont, yGui,1,1,1);
	gridGui->addWidget(btnFont, yGui++,2,1,1);
	gridGui->addWidget(m_checkMenubar, yGui++,0,1,3);
	gridGui->addWidget(m_checkDialogs, yGui++,0,1,3);

	QSpacerItem *spacer_end = new QSpacerItem(1, 1,
		QSizePolicy::Minimum, QSizePolicy::Expanding);
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
	buttons->setStandardButtons(QDialogButtonBox::Ok |
		QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
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

	// search items
	connect(editSearch, &QLineEdit::textChanged, [editSearch, this]()
	{
		QList<QTableWidgetItem*> items =
			m_table->findItems(editSearch->text(), Qt::MatchContains);

		/*
		// unselect all items
		for(int row=0; row<m_table->rowCount(); ++row)
		{
			m_table->item(row, 0)->setSelected(false);
			m_table->item(row, 1)->setSelected(false);
			m_table->item(row, 2)->setSelected(false);
		}

		// select all found items
		for(auto* item : items)
			item->setSelected(true);
		*/

		// scroll to first found item
		if(items.size())
			m_table->setCurrentItem(items[0]);
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

	auto seq = std::make_index_sequence<g_settingsvariables.size()>();
	get_settings_loop(seq, sett);

	get_setting<decltype(g_theme)>(sett, "settings/theme", &g_theme);
	get_setting<decltype(g_font)>(sett, "settings/font", &g_font);
	get_setting<decltype(g_use_native_menubar)>(sett, "settings/native_menubar", &g_use_native_menubar);
	get_setting<decltype(g_use_native_dialogs)>(sett, "settings/native_dialogs", &g_use_native_dialogs);

	ApplyGuiSettings();
}


/**
 * 'Apply' was clicked, write the settings from the global variables
 */
void SettingsDlg::ApplySettings()
{
	auto seq = std::make_index_sequence<g_settingsvariables.size()>();
	apply_settings_loop(seq, m_table, m_sett);

	// set the global variables
	g_theme = m_comboTheme->currentText();
	g_font = m_editFont->text();
	g_use_native_menubar = m_checkMenubar->isChecked();
	g_use_native_dialogs = m_checkDialogs->isChecked();

	// write out the settings
	if(m_sett)
	{
		m_sett->setValue("settings/theme", g_theme);
		m_sett->setValue("settings/font", g_font);
		m_sett->setValue("settings/native_menubar", g_use_native_menubar);
		m_sett->setValue("settings/native_dialogs", g_use_native_dialogs);
	}

	ApplyGuiSettings();
	emit SettingsHaveChanged();
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

	// set native menubar and dialogs
	QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, !g_use_native_menubar);
	QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, !g_use_native_dialogs);
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
