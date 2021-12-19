/**
 * texture browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 19-dec-2021
 * @license GPLv3, see 'LICENSE' file
 * @note Forked on 19-dec-2021 from my privately developed "misc" project (https://github.com/t-weber/misc).
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

#include "TextureBrowser.h"
#include "settings_variables.h"

#include <QtGui/QPainter>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QDialogButtonBox>



// ----------------------------------------------------------------------------
void ImageWidget::SetImage(const QString& img)
{
	if(img.isEmpty())
		return;

	if(m_img.load(img))
		update();
}


void ImageWidget::paintEvent(QPaintEvent *evt)
{
	QWidget::paintEvent(evt);

	if(!m_img.isNull())
	{
		QPainter painter{};
		painter.begin(this);
		painter.drawPixmap(0,0,width(), height(), m_img);
		painter.end();
	}
}
// ----------------------------------------------------------------------------



// ---------------------------------------------------------------------------- 
TextureBrowser::TextureBrowser(QWidget* pParent, QSettings *sett)
	: QDialog{pParent}, m_sett(sett)
{
	setWindowTitle("Texture Browser");
	setSizeGripEnabled(true);

	// list widget
	m_list = new QListWidget(this);
	m_list->setSortingEnabled(true);
	m_list->setMouseTracking(true);

	QPushButton *btnAddImage = new QPushButton("Add Image...", this);

	// list widget grid
	QWidget *widget_list = new QWidget(this);
	auto grid_list = new QGridLayout(widget_list);
	grid_list->setSpacing(2);
	grid_list->setContentsMargins(4,4,4,4);
	grid_list->addWidget(m_list, 0,0,1,1);
	grid_list->addWidget(btnAddImage, 1,0,1,1);

	// image widget
	m_image = new ImageWidget(this);

	// standard buttons
	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Ok);

	// splitter
	m_splitter = new QSplitter(Qt::Horizontal, this);
	m_splitter->addWidget(widget_list);
	m_splitter->addWidget(m_image);
	m_splitter->setStretchFactor(0, 1);
	m_splitter->setStretchFactor(1, 4);

	// grid
	auto grid_dlg = new QGridLayout(this);
	grid_dlg->setSpacing(4);
	grid_dlg->setContentsMargins(12,12,12,12);
	grid_dlg->addWidget(m_splitter, 0,0,1,1);
	grid_dlg->addWidget(buttons, 1,0,1,1);

	// restore settings
	if(m_sett)
	{
		// restore dialog geometry
		if(m_sett->contains("texturebrowser/geo"))
			restoreGeometry(m_sett->value("texturebrowser/geo").toByteArray());
		else
			resize(600, 400);

		// restore splitter position
		if(m_sett->contains("texturebrowser/splitter"))
			m_splitter->restoreState(m_sett->value("texturebrowser/splitter").toByteArray());
	}


	// connections
	connect(m_list, &QListWidget::currentItemChanged,
		this, &TextureBrowser::ListItemChanged);
	connect(btnAddImage, &QAbstractButton::clicked,
		this, &TextureBrowser::BrowseImageFiles);

	connect(buttons, &QDialogButtonBox::accepted,
		this, &TextureBrowser::accept);
	connect(buttons, &QDialogButtonBox::rejected,
		this, &TextureBrowser::reject);
}


TextureBrowser::~TextureBrowser()
{
}


void TextureBrowser::BrowseImageFiles()
{
	QString dirLast = g_imgpath.c_str();;
	if(m_sett)
		dirLast = m_sett->value("cur_texture_dir", dirLast).toString();

	QFileDialog filedlg(this, "Open Image File", dirLast,
		"Images (*.png *.jpg)");
	filedlg.setAcceptMode(QFileDialog::AcceptOpen);
	filedlg.setDefaultSuffix("taspaths");
	filedlg.setViewMode(QFileDialog::Detail);
	filedlg.setFileMode(QFileDialog::ExistingFiles);
	filedlg.setSidebarUrls(QList<QUrl>({
		QUrl::fromLocalFile(g_homepath.c_str()),
		QUrl::fromLocalFile(g_desktoppath.c_str()),
		QUrl::fromLocalFile(g_docpath.c_str())}));

	if(!filedlg.exec())
		return;

	QStringList files = filedlg.selectedFiles();
	for(const QString& file : files)
		m_list->addItem(file);

	if(m_sett && files.size())
		m_sett->setValue("cur_texture_dir", QFileInfo(files[0]).path());
}


void TextureBrowser::ListItemChanged(
	QListWidgetItem* cur, [[maybe_unused]] QListWidgetItem* prev)
{
	m_image->SetImage(cur->text());
}


/**
 * close the dialog
 */
void TextureBrowser::accept()
{
	if(m_sett)
	{
		// save dialog geometry
		m_sett->setValue("texturebrowser/geo", saveGeometry());
		m_sett->setValue("texturebrowser/splitter", m_splitter->saveState());
	}

	QDialog::accept();
}
// ---------------------------------------------------------------------------- 
