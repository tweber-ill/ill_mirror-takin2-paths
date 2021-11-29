/**
 * TAS paths tool -- settings dialog
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
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QFontDialog>

#include <type_traits>

#include "settings_variables.h"
#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/str.h"
#include "tlibs2/libs/qt/numerictablewidgetitem.h"


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

	QTableWidgetItem *item = new NumericTableWidgetItem<t_value>(finalval, 10);
	table->setItem((int)idx, 0, new QTableWidgetItem{var.description});
	table->setItem((int)idx, 1, new QTableWidgetItem{type_str<t_value>});
	table->setItem((int)idx, 2, item);

	if(var.editor == SettingsVariableEditor::YESNO)
	{
		QComboBox *combo = new QComboBox(table);
		combo->addItem("No");
		combo->addItem("Yes");

		combo->setCurrentIndex(finalval==0 ? 0 : 1);
		table->setCellWidget((int)idx, 2, combo);
	}
	if(var.editor == SettingsVariableEditor::COMBOBOX)
	{
		std::vector<std::string> config_tokens;
		tl2::get_tokens_seq<std::string, std::string>(
			var.editor_config, ";;", config_tokens, true);

		QComboBox *combo = new QComboBox(table);
		for(const std::string& config_token : config_tokens)
			combo->addItem(config_token.c_str());

		combo->setCurrentIndex((int)finalval);
		table->setCellWidget((int)idx, 2, combo);
	}
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
 * save the current settings value as its default value
 */
template<std::size_t idx>
static void save_default_value(
	std::unordered_map<std::string, SettingsVariable::t_variant>& map)
{
	constexpr const SettingsVariable& var = std::get<idx>(g_settingsvariables);
	constexpr auto* value = std::get<var.value.index()>(var.value);

	map.insert_or_assign(var.key, *value);
}


/**
 * save the current settings values as their default values
 */
template<std::size_t ...seq>
static constexpr void save_default_values_loop(
	const std::index_sequence<seq...>&,
	std::unordered_map<std::string, SettingsVariable::t_variant>& map)
{
	// a sequence of function calls
	( (save_default_value<seq>(map)), ... );
}


/**
 * restore the current settings value from its default value
 */
template<std::size_t idx>
static void restore_default_value(
	const std::unordered_map<std::string, SettingsVariable::t_variant>& map)
{
	constexpr const SettingsVariable& var = std::get<idx>(g_settingsvariables);
	constexpr auto* value = std::get<var.value.index()>(var.value);
	using t_value = std::decay_t<decltype(*value)>;

	// does the defaults map have this key?
	if(auto iter = map.find(std::string{var.key}); iter!=map.end())
	{
		// does the settings variable variant hold this type?
		if(std::holds_alternative<t_value>(iter->second))
			*value = std::get<t_value>(iter->second);
	}
}


/**
 * restore the current settings values from their default values
 */
template<std::size_t ...seq>
static constexpr void restore_default_values_loop(
	const std::index_sequence<seq...>&,
	const std::unordered_map<std::string, SettingsVariable::t_variant>& map)
{
	// a sequence of function calls
	( (restore_default_value<seq>(map)), ... );
}


/**
 * reads a settings item from the table and saves it to the
 * corresponding global variable and to the QSettings object
 */
template<std::size_t idx>
static void apply_settings_item(QTableWidget *table, QSettings *sett)
{
	constexpr const SettingsVariable& var = std::get<idx>(g_settingsvariables);
	constexpr auto* value = std::get<var.value.index()>(var.value);
	using t_value = std::decay_t<decltype(*value)>;

	t_value finalval = dynamic_cast<NumericTableWidgetItem<t_value>*>(
		table->item((int)idx, 2))->GetValue();
	if(var.is_angle)
		finalval = finalval / 180.*tl2::pi<t_real>;

	// alternatively use the value from the editor widget if available
	if(var.editor == SettingsVariableEditor::YESNO ||
		var.editor == SettingsVariableEditor::COMBOBOX)
	{
		QComboBox *combo = static_cast<QComboBox*>(
			table->cellWidget((int)idx, 2));
		finalval = (t_value)combo->currentIndex();
	}

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



// ----------------------------------------------------------------------------
// settings dialog
// ----------------------------------------------------------------------------

// default setting values
std::unordered_map<std::string, SettingsVariable::t_variant> SettingsDlg::s_defaults = {};


SettingsDlg::SettingsDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Settings");

	// table column widths
	int col0_w = 200;
	int col1_w = 100;
	int col2_w = 150;

	if(m_sett)
	{
		// restore dialog geometry
		if(m_sett->contains("settings/geo"))
			restoreGeometry(m_sett->value("settings/geo").toByteArray());
		else
			resize(512, 425);

		// restore table column widths
		if(m_sett->contains("settings/col0_width"))
			col0_w = m_sett->value("settings/col0_width").toInt();
		if(m_sett->contains("settings/col1_width"))
			col1_w = m_sett->value("settings/col1_width").toInt();
		if(m_sett->contains("settings/col2_width"))
			col2_w = m_sett->value("settings/col2_width").toInt();
	}

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
	m_table->setColumnWidth(0, col0_w);
	m_table->setColumnWidth(1, col1_w);
	m_table->setColumnWidth(2, col2_w);
	m_table->setHorizontalHeaderItem(0, new QTableWidgetItem{"Setting"});
	m_table->setHorizontalHeaderItem(1, new QTableWidgetItem{"Type"});
	m_table->setHorizontalHeaderItem(2, new QTableWidgetItem{"Value"});

	// table contents
	PopulateSettingsTable();


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
	m_checkMenubar = new QCheckBox("Use native menubar.", panelGui);
	get_setting<decltype(g_use_native_menubar)>(sett, "settings/native_menubar", &g_use_native_menubar);
	m_checkMenubar->setChecked(g_use_native_menubar!=0);

	// native dialogs
	m_checkDialogs = new QCheckBox("Use native dialogs.", panelGui);
	get_setting<decltype(g_use_native_dialogs)>(sett, "settings/native_dialogs", &g_use_native_dialogs);
	m_checkDialogs->setChecked(g_use_native_dialogs!=0);

	// gui animations
	m_checkAnimations = new QCheckBox("Use animations.", panelGui);
	get_setting<decltype(g_use_animations)>(sett, "settings/animations", &g_use_animations);
	m_checkAnimations->setChecked(g_use_animations!=0);

	// tabbed docks
	m_checkTabbedDocks = new QCheckBox("Allow tabbed dock widgets.", panelGui);
	get_setting<decltype(g_tabbed_docks)>(sett, "settings/tabbed_docks", &g_tabbed_docks);
	m_checkTabbedDocks->setChecked(g_tabbed_docks!=0);

	// nested docks
	m_checkNestedDocks = new QCheckBox("Allow nested dock widgets.", panelGui);
	get_setting<decltype(g_nested_docks)>(sett, "settings/nested_docks", &g_nested_docks);
	m_checkNestedDocks->setChecked(g_nested_docks!=0);


	// add widgets to layout
	gridGui->addWidget(labelTheme, yGui,0,1,1);
	gridGui->addWidget(m_comboTheme, yGui++,1,1,2);
	gridGui->addWidget(labelFont, yGui,0,1,1);
	gridGui->addWidget(m_editFont, yGui,1,1,1);
	gridGui->addWidget(btnFont, yGui++,2,1,1);
	gridGui->addWidget(m_checkMenubar, yGui++,0,1,3);
	gridGui->addWidget(m_checkDialogs, yGui++,0,1,3);
	gridGui->addWidget(m_checkAnimations, yGui++,0,1,3);
	gridGui->addWidget(m_checkTabbedDocks, yGui++,0,1,3);
	gridGui->addWidget(m_checkNestedDocks, yGui++,0,1,3);

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
	buttons->setStandardButtons(
		QDialogButtonBox::Ok |
		QDialogButtonBox::Apply |
		QDialogButtonBox::RestoreDefaults |
		QDialogButtonBox::Cancel);
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

	connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDlg::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDlg::reject);
	connect(buttons, &QDialogButtonBox::clicked, [this, buttons](QAbstractButton* btn)
	{
		// apply button was pressed
		if(btn == static_cast<QAbstractButton*>(buttons->button(QDialogButtonBox::Apply)))
			ApplySettings();
		else if(btn == static_cast<QAbstractButton*>(buttons->button(QDialogButtonBox::RestoreDefaults)))
			RestoreDefaultSettings();
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
 * populate the settings table using the global settings items
 */
void SettingsDlg::PopulateSettingsTable()
{
	m_table->clearContents();
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
}


/**
 * read the settings and set the global variables
 */
void SettingsDlg::ReadSettings(QSettings* sett)
{
	// save the initial values as default settings
	static bool first_run = true;
	if(first_run)
	{
		SaveDefaultSettings();
		first_run = false;
	}

	if(!sett)
		return;

	auto seq = std::make_index_sequence<g_settingsvariables.size()>();
	get_settings_loop(seq, sett);

	get_setting<decltype(g_theme)>(sett, "settings/theme", &g_theme);
	get_setting<decltype(g_font)>(sett, "settings/font", &g_font);
	get_setting<decltype(g_use_native_menubar)>(sett, "settings/native_menubar", &g_use_native_menubar);
	get_setting<decltype(g_use_native_dialogs)>(sett, "settings/native_dialogs", &g_use_native_dialogs);
	get_setting<decltype(g_use_animations)>(sett, "settings/animations", &g_use_animations);
	get_setting<decltype(g_tabbed_docks)>(sett, "settings/tabbed_docks", &g_tabbed_docks);
	get_setting<decltype(g_nested_docks)>(sett, "settings/nested_docks", &g_nested_docks);

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
	g_use_animations = m_checkAnimations->isChecked();
	g_tabbed_docks = m_checkTabbedDocks->isChecked();
	g_nested_docks = m_checkNestedDocks->isChecked();

	// write out the settings
	if(m_sett)
	{
		m_sett->setValue("settings/theme", g_theme);
		m_sett->setValue("settings/font", g_font);
		m_sett->setValue("settings/native_menubar", g_use_native_menubar);
		m_sett->setValue("settings/native_dialogs", g_use_native_dialogs);
		m_sett->setValue("settings/animations", g_use_animations);
		m_sett->setValue("settings/tabbed_docks", g_tabbed_docks);
		m_sett->setValue("settings/nested_docks", g_nested_docks);
	}

	ApplyGuiSettings();
	emit SettingsHaveChanged();
}


/**
 * save the current setting values as default values
 */
void SettingsDlg::SaveDefaultSettings()
{
	auto seq = std::make_index_sequence<g_settingsvariables.size()>();
	save_default_values_loop(seq, s_defaults);
}


/**
 * 'Restore Defaults' was clicked, restore original settings
 */
void SettingsDlg::RestoreDefaultSettings()
{
	auto seq = std::make_index_sequence<g_settingsvariables.size()>();
	restore_default_values_loop(seq, s_defaults);

	// re-populte the settings table
	PopulateSettingsTable();
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
	{
		// save dialog geometry
		m_sett->setValue("settings/geo", saveGeometry());

		// save table column widths
		if(m_table)
		{
			m_sett->setValue("settings/col0_width", m_table->columnWidth(0));
			m_sett->setValue("settings/col1_width", m_table->columnWidth(1));
			m_sett->setValue("settings/col2_width", m_table->columnWidth(2));
		}
	}

	QDialog::accept();
}
// ----------------------------------------------------------------------------
