/* Pango
 * viewer-qt.cc: Example program to view a UTF-8 encoding file
 *               using Pango to render result.
 *
 * Copyright (C) 1999-2000 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <qscrollview.h>
#include <qlist.h>
#include <qmainwindow.h>
#include <pango/pango.h>

class QComboBox;
class QSpinBox;

class ViewerPara 
{
 public:
  ViewerPara (PangoContext *context, const QString &text);
  ~ViewerPara ();

  void setWidth (int width);
  void contextChanged ();
  int height ();
  void draw (QPainter *painter, GC gc, int y);
  QRect charBounds (int index);
  gunichar getChar (int index);
  int findPoint (int x, int y);

 private:
  QCString text_;
  int height_;
  PangoLayout *layout_;
};

class ViewerView : public QScrollView 
{
  Q_OBJECT

 public:

  ViewerView::ViewerView (QWidget *parent, QStatusBar *status = NULL);
  ~ViewerView ();

  void readFile (const QString &name);
  void setFontDesc (PangoFontDescription *font_desc);
  void setDirection (PangoDirection base_dir);

  void computeHeight ();

  PangoContext *context() { return context_; }

 protected:
  virtual void drawContents (QPainter *p, int clipx, int clipy, int clipw, int cliph);
  virtual void viewportResizeEvent ( QResizeEvent *event );
  virtual void contentsMousePressEvent (QMouseEvent *event);

 private:
  void contextChanged ();
  void updateHighlightChar ();

  QStatusBar *status_;

  PangoContext *context_;
  QList<ViewerPara> paragraphs_;

  ViewerPara *highlight_para_;
  int highlight_index_;
};

class ViewerWindow : public QMainWindow 
{
  Q_OBJECT

 public:

  ViewerWindow ();
  ~ViewerWindow ();

 private:
  ViewerView *view_;

  PangoContext *context_;

  QComboBox *family_combo_;
  QComboBox *style_combo_;
  QSpinBox *size_box_;

  QPopupMenu *file_menu_;

  int right_to_left_item_;

 private slots:
    void fillStyleCombo (int index);
    void fontChanged ();
    void setRightToLeft ();
};
