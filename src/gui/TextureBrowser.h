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

#ifndef __TASPATHS_TEXTURE_BROWSER_H__
#define __TASPATHS_TEXTURE_BROWSER_H__

#include <QtCore/QSettings>
#include <QtGui/QPixmap>
#include <QtWidgets/QDialog>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSplitter>

#include <vector>
#include <memory>


class ImageWidget : public QFrame
{
public:
	ImageWidget(QWidget* parent);
	virtual ~ImageWidget() = default;

	void SetImage(const QString& img);

protected:
	virtual void paintEvent(QPaintEvent *evt) override;

private:
	QPixmap m_img{};
};



class TextureBrowser : public QDialog
{ Q_OBJECT
public:
	TextureBrowser(QWidget* pParent = nullptr, QSettings *sett = nullptr);
	virtual ~TextureBrowser();

	TextureBrowser(const TextureBrowser&) = delete;
	const TextureBrowser& operator=(const TextureBrowser&) = delete;


protected:
	virtual void accept() override;

	void ListItemChanged(QListWidgetItem* cur, QListWidgetItem* prev);
	void BrowseImageFiles();


private:
	QSettings *m_sett{nullptr};

	QSplitter *m_splitter{nullptr};
	QListWidget *m_list{nullptr};
	ImageWidget *m_image{nullptr};


signals:
	void SignalEnableTextures(bool);
	void SignalChangeTexture(std::size_t idx, const QString& filename);
};


#endif
