/*
   Copyright (C) 2003,2004 George Staikos <staikos@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kwmapeditor.h"
#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmenu.h>
#include <kstandardaction.h>
#include <kwindowsystem.h>
#include <QPointer>
#include <QApplication>
#include <QClipboard>
#include <QPushButton>
#include <ktextedit.h>
#include <QFocusEvent>
#include <QKeyEvent>

KWMapEditor::KWMapEditor(QMap<QString,QString>& map, QWidget *parent, const char *name)
: Q3Table(0, 3, parent, name), _map(map) {
	_ac = new KActionCollection(this);
	_copyAct = KStandardAction::copy(this, SLOT(copy()), _ac);
	connect(this, SIGNAL(valueChanged(int,int)), this, SIGNAL(dirty()));
	connect(this, SIGNAL(contextMenuRequested(int,int,const QPoint&)),
		this, SLOT(contextMenu(int,int,const QPoint&)));
	setSelectionMode(Q3Table::NoSelection);
	horizontalHeader()->setLabel(0, QString());
	horizontalHeader()->setLabel(1, i18n("Key"));
	horizontalHeader()->setLabel(2, i18n("Value"));
	setColumnWidth(0, 20); // FIXME: this is arbitrary
	reload();
}

void KWMapEditor::reload() {
	int row = 0;

	while ((row = numRows()) > _map.count()) {
		removeRow(row - 1);
	}

	if ((row = numRows()) < _map.count()) {
		insertRows(row, _map.count() - row);
		for (int x = row; x < numRows(); ++x) {
			QPushButton *b = new QPushButton("X", this);
			connect(b, SIGNAL(clicked()), this, SLOT(erase()));
			setCellWidget(x, 0, b);
		}
	}

	row = 0;
	for (QMap<QString,QString>::Iterator it = _map.begin(); it != _map.end(); ++it) {
		setText(row, 1, it.key());
		setText(row, 2, it.value());
		row++;
	}
}


KWMapEditor::~KWMapEditor() {
}


void KWMapEditor::erase() {
	const QObject *o = sender();
	for (int i = 0; i < numRows(); i++) {
		if (cellWidget(i, 0) == o) {
			removeRow(i);
			break;
		}
	}

	emit dirty();
}


void KWMapEditor::saveMap() {
	_map.clear();

	for (int i = 0; i < numRows(); i++) {
		_map[text(i, 1)] = text(i, 2);
	}
}


void KWMapEditor::addEntry() {
	int x = numRows();
	insertRows(x, 1);
	QPushButton *b = new QPushButton("X", this);
	connect(b, SIGNAL(clicked()), this, SLOT(erase()));
	setCellWidget(x, 0, b);
	ensureCellVisible(x, 1);
	setCurrentCell(x, 1);
	emit dirty();
}


void KWMapEditor::emitDirty() {
	emit dirty();
}


void KWMapEditor::contextMenu(int row, int col, const QPoint& pos) {
	_contextRow = row;
	_contextCol = col;
	KMenu *m = new KMenu(this);
	m->addAction(i18n("&New Entry"), this, SLOT(addEntry()));
	m->addAction( _copyAct );
	m->popup(pos);
}


void KWMapEditor::copy() {
	QApplication::clipboard()->setText(text(_contextRow, 2));
}


class InlineEditor : public KTextEdit {
	public:
	InlineEditor(KWMapEditor *p, int row, int col) : KTextEdit(), _p(p), row(row), col(col) {
		setAttribute(Qt::WA_DeleteOnClose);
		setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
		connect(p, SIGNAL(destroyed()), SLOT(close()));
	}
	virtual ~InlineEditor() {
		if (!_p) return;
		_p->setText(row, col, toPlainText()); _p->emitDirty();
	}

	protected:
		virtual void focusOutEvent(QFocusEvent *e) {
			if (e->reason() == Qt::PopupFocusReason) {
				// TODO: It seems we only get here if we're disturbed
				// by our own popup. this needs some clearance though.
				return;
			}

			close();
		}
		virtual void keyPressEvent(QKeyEvent *e) {
			if (e->key() == Qt::Key_Escape) {
				e->accept();
				close();
			} else {
				e->ignore();
				KTextEdit::keyPressEvent(e);
			}
		}
		virtual void contextMenuEvent( QContextMenuEvent *event )
		{
		   QMenu *menu = createStandardContextMenu();
		   popup = menu;
		   popup->exec( event->globalPos() );
		   delete popup;
		}
		QPointer<KWMapEditor> _p;
		int row, col;
		QPointer<QMenu> popup;
};

QWidget *KWMapEditor::beginEdit(int row, int col, bool replace) {
	//kDebug() << "EDIT COLUMN " << col ;
	if (col != 2) {
		return Q3Table::beginEdit(row, col, replace);
	}

	QRect geo = cellGeometry(row, col);
	KTextEdit *e = new InlineEditor(this, row, col);
	e->setCheckSpellingEnabled(false); // disable spell-checking
	e->setText(text(row, col));
	e->move(mapToGlobal(geo.topLeft()));
	e->resize(geo.width() * 2, geo.height() * 3);
	e->show();
	e->setFocus(Qt::PopupFocusReason);
	return e;
}


#include "kwmapeditor.moc"
